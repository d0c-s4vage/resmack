#include <cstring>
#include <chrono>
#include <cxxabi.h>
#include <csignal>
#include <cstdlib>
#include <libunwind-x86_64.h>
#include <mutex>
#include <libunwind-ptrace.h>
#include <pthread.h>
#include <signal.h>
#include <stdexcept>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <thread>
#include <time.h>
#include <unistd.h>

#include "resmack/debug.hpp"
#include "resmack/fuzz/asan_util.hpp"
#include "resmack/rand.hpp"
#include "resmack/utils.hpp"
#include "resmack/fuzz/tracer.hpp"
#include "resmack/fuzz/utils.hpp"
#include "resmack/fuzz/process_utils.hpp"


namespace resmack {
namespace fuzz {
  Tracer::Tracer(
    ProcessLauncher* proc_launcher,
    TraceExceptionCb exception_cb,
    TraceTimeoutCb timeout_cb,
    uint32_t idx,
    bool using_asan
  ) :
    idx_(idx),
    using_asan_(using_asan),
    tracee_(idx),
    proc_launcher_(proc_launcher),
    traced_pid_(-1),
    timeout_(5.0),
    exception_cb_(exception_cb),
    timeout_cb_(timeout_cb),
    trace_lock_("TraceLock", true)
  {
    DEBUG_PRINT("USING ASAN: %d\n", using_asan_);
  }
  Tracer::~Tracer() {}

  void Tracer::Start() {
    should_run_.store(true);
    monitor_thread_ = utils::CreateThread(&Tracer::MonitorTracee, this);
    monitor_timeout_thread_ = utils::CreateThread(&Tracer::MonitorTraceeTimeout, this);
  }

  void Tracer::Stop(bool hard_stop) {
    should_run_.store(false);

    if (hard_stop) {
      pid_t pid = traced_pid_.load();
      if (pid > 0) {
        DEBUG_PRINT("KILLING THE TRACED PID: %d\n", pid);
        kill(pid, SIGKILL);
      }
    }
  }

  void Tracer::Join() {
    pthread_join(monitor_thread_, nullptr);
    pthread_join(monitor_timeout_thread_, nullptr);
  }

  void* Tracer::DoMonitorTraceeTimeout(void* this_arg) {
    static_cast<Tracer*>(this_arg)->MonitorTraceeTimeout();
    return nullptr;
  }

  void* Tracer::DoMonitorTracee(void* this_arg) {
    static_cast<Tracer*>(this_arg)->MonitorTracee();
    return nullptr;
  }

  /** This is run once per Tracer, and monitors the currently-running
   * single Tracee that the Tracer is ... tracing haha.
   *
   * Each time this thread wakes up, it checks how long the current
   * tracee has been running. If it's been longer than the timeout,
   * then it kills the tracee.
   **/
  void Tracer::MonitorTraceeTimeout() {
    float alive_max = 10000.0f;

    while (should_run_.load()) {
      {
        std::scoped_lock _lock(trace_lock_);
        pid_t pid = traced_pid_.load();
        // hasn't started yet
        if (tracee_.GetIterStart() <= 0.0f) {
          ; // noop
        } else {
          float now = utils::GetTimeNow();
          float iter_span = now  - tracee_.GetIterStart();
          float lifetime_span = now - tracee_.GetLifetimeStart();

          tracee_.iter_timed_out_ = (iter_span >= timeout_);
          tracee_.lifetime_timed_out_ = (lifetime_span >= alive_max);
          if (tracee_.iter_timed_out_ || tracee_.lifetime_timed_out_) {
            DEBUG_PRINT("KILLING THE TRACED PID (TIMEDOUT): %d\n", pid);
            kill(pid, SIGKILL);
            break;
          }
        }
      } // std::scoped_lock

      auto ms = std::chrono::milliseconds(100);
      std::this_thread::sleep_for(ms);
    } // while

    DEBUG_PRINT("Exiting Monitor Trace Timeout thread\n");
  }

  process_utils::SignalInfo Tracer::WaitForExit() {
    pid_t pid = traced_pid_.load();
    int status;
    process_utils::SignalInfo sig_info;

    bool keep_going = true;
    while (keep_going) {
      keep_going = false;

      if (waitpid(pid, &status, 0) == -1) {
        // the timeout thread may have killed it
        if (errno == ECHILD) {
          sig_info.exit_reason = process_utils::ExitReason::Normal;
          sig_info.exited = true;
          return sig_info;
        }
        throw std::runtime_error("Error waiting for child: " + std::string(std::strerror(errno)));
      }
      sig_info = process_utils::ExtractSignalInfo(status);

      if (using_asan_) {
        DEBUG_PRINT("USING ASAN\n");
        // it's an event forwarded by ptrace, we need to continue so the signal gets
        // processed by ASAN
        if (sig_info.stopped) {
          keep_going = true;

        // a definitive ASAN exit
        } else if (sig_info.exited && sig_info.exit_status == asan::ASAN_EXIT_CODE) {
          keep_going = false;

        // all others
        } else {
          DEBUG_PRINT("WHAT WAS THIS????\n");
          keep_going = false;
        }
      } else {
        keep_going = (sig_info.exit_reason == process_utils::ExitReason::Ptrace);
      }

      if (keep_going) {
        DEBUG_PRINT("KEEP_GOING: YES, sending PTRACE_CONT\n");
        if (ptrace(PTRACE_CONT, pid, nullptr, sig_info.stop_signal) == -1) {
          throw std::runtime_error("Could not tell traced process to continue: " + std::string(std::strerror(errno)));
        }
      }
    }

    return sig_info;
  }

  pid_t Tracer::LaunchTargetProcess() {
    return proc_launcher_->Spawn(&tracee_);
  }

  void Tracer::ProcessCrash(process_utils::SignalInfo sig_info) {
    DEBUG_PRINT("PROCESSING CRASH\n");
    last_crash_.crashed = true;
    last_crash_.signal_info = sig_info;

    const ser::AsanInfo* asan_info = tracee_.GetAsanInfo();

    // simple case: we used ASAN and just need to copy the info
    if (asan_info != nullptr) {
      last_crash_.major_stack.clear();
      last_crash_.minor_stack.clear();
      memcpy(last_crash_.major_hash, asan_info->major_hash, sizeof(asan_info->major_hash));
      memcpy(last_crash_.minor_hash, asan_info->minor_hash, sizeof(asan_info->minor_hash));


    // more complicated: we didn't use ASAN and we need to calculate / collect the info manually
    } else {
      last_crash_.major_hash[0] = 0;
      last_crash_.minor_hash[0] = 0;

      CalcHashes();

      if (last_crash_.major_hash[0] == 0) {
        Rand rand;
        std::string charset = "abcdefghijklmnopqrstuvwxyz";

        snprintf(last_crash_.major_hash, sizeof(last_crash_.major_hash), "%s", "unknown_exit");
        last_crash_.major_stack = "unknown_exit";
        last_crash_.minor_stack = "unknown_exit";
        std::string out;
        resmack::utils::RandBytes(&rand, charset.c_str(), 10, &out);
        snprintf(last_crash_.minor_hash, sizeof(last_crash_.minor_hash), "%s", out.c_str());
      }
    }
  }

  void Tracer::MonitorTracee() {
    while (should_run_.load()) {
      traced_pid_.store(LaunchTargetProcess());
      process_utils::SignalInfo exit_info = WaitForExit();

      if (!should_run_.load()) {
        break;
      }

      switch(exit_info.exit_reason) {
        case process_utils::ExitReason::Normal:
          DEBUG_PRINT("MonitorTracee:: Normal exit\n");
          continue;
        case process_utils::ExitReason::Crash:
          DEBUG_PRINT("MonitorTracee:: Crashing exit\n");
          ProcessCrash(exit_info);
          exception_cb_(traced_pid_.load(), this, &tracee_);

          // this happens if we aren't using ASAN
          if (exit_info.stopped && !exit_info.exited) {
            pid_t pid = traced_pid_.load();
            kill(pid, SIGKILL);

            int reap_status;
            // don't leave it hanging (reap it)
            waitpid(pid, &reap_status, 0);
          }

          break;
        case process_utils::ExitReason::Timeout:
          DEBUG_PRINT("MonitorTracee:: Timeout exit\n");
          timeout_cb_(traced_pid_.load(), this, &tracee_);
          break;
        default:
          break;
      }
    }
  }

  // https://github.com/daniel-thompson/libunwind-examples/blob/master/unwind-pid.c
  void Tracer::CalcHashes() {
    last_crash_.major_stack.clear();
    last_crash_.minor_stack.clear();

    unw_addr_space_t as = unw_create_addr_space(&_UPT_accessors, 0);
    void *context = _UPT_create(traced_pid_.load());
    unw_cursor_t cursor;
    int res = unw_init_remote(&cursor, as, context);
    if (res != 0) {
      _UPT_destroy(context);
      unw_destroy_addr_space(as);
      throw std::runtime_error(std::format("Could not create stack-unwinding cursor: {}", res));
    }

    // last five frames
    std::string* major_stack = &last_crash_.major_stack;
    // all frames
    std::string* minor_stack = &last_crash_.minor_stack;

    char sym[4096];
    size_t count = 0;
    do {
      count++;
      unw_word_t offset;
      unw_word_t pc;

      res = unw_get_reg(&cursor, UNW_REG_IP, &pc);
      if (res != 0) {
        _UPT_destroy(context);
        unw_destroy_addr_space(as);
        throw std::runtime_error(std::format("Could not create stack-unwinding cursor: {}", res));
        //return;
      }

      int res = unw_get_proc_name(&cursor, sym, sizeof(sym), &offset);
      if (res == 0 || res == UNW_ENOMEM) {
        int status;
        size_t demangled_size;
        char* demangled = abi::__cxa_demangle(sym, nullptr, &demangled_size, &status);
        if (demangled != nullptr) {
          snprintf(sym, sizeof(sym), "%s+0x%lx", demangled, offset);
          free(demangled);
        } else {
          snprintf(sym + strlen(sym), sizeof(sym) - strlen(sym), "+0x%lx", offset);
        }
      } else {
        snprintf(sym, sizeof(sym), "??");
      }

      if (count <= 5) {
        if (count > 0) { *major_stack += "\n"; }
        *major_stack += sym;
      }
      if (count > 1) { *minor_stack += "\n"; }
      *minor_stack += sym;

      if (strstr(sym, "LLVMFuzzerTestOneInput") != nullptr) {
        break;
      }
    } while (unw_step(&cursor) > 0);

    _UPT_destroy(context);
    unw_destroy_addr_space(as);

    utils::sha1_hex(major_stack->data(), major_stack->size(), last_crash_.major_hash);
    utils::sha1_hex(minor_stack->data(), minor_stack->size(), last_crash_.minor_hash);
  }

}
}
