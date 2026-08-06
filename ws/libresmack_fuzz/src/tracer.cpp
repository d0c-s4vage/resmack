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
    uint32_t idx
  ) :
    idx(idx),
    tracee(idx),
    proc_launcher(proc_launcher),
    traced_pid(-1),
    timeout(5.0),
    exception_cb(exception_cb),
    timeout_cb(timeout_cb),
    trace_lock("TraceLock", true)
  {}
  Tracer::~Tracer() {}

  void Tracer::Start() {
    should_run.store(true);
    monitor_thread = utils::CreateThread(&Tracer::MonitorTracee, this);
    monitor_timeout_thread = utils::CreateThread(&Tracer::MonitorTraceeTimeout, this);
  }

  void Tracer::Stop(bool hard_stop) {
    should_run.store(false);

    if (hard_stop) {
      pid_t pid = traced_pid.load();
      if (pid > 0) {
        DEBUG_PRINT("KILLING THE TRACED PID: %d\n", pid);
        kill(pid, SIGKILL);
      }
    }
  }

  void Tracer::Join() {
    pthread_join(monitor_thread, nullptr);
    pthread_join(monitor_timeout_thread, nullptr);
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
    float alive_max = 600.0f;

    while (should_run.load()) {
      {
        std::scoped_lock _lock(trace_lock);
        pid_t pid = traced_pid.load();
        // hasn't started yet
        if (tracee.GetIterStart() <= 0.0f) {
          ; // noop
        } else {
          float now = utils::GetTimeNow();
          float iter_span = now  - tracee.GetIterStart();
          float lifetime_span = now - tracee.GetLifetimeStart();

          tracee.iter_timed_out = (iter_span >= timeout);
          tracee.lifetime_timed_out = (lifetime_span >= alive_max);
          if (tracee.iter_timed_out || tracee.lifetime_timed_out) {
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
    int status;
    if (waitpid(traced_pid.load(), &status, 0) == -1) {
      DEBUG_PRINT("Error waiting for traced pid\n");
      // the process is already dead!
      //if (errno == ECHILD) {
      //}
      throw std::runtime_error("Error waiting for child: " + std::string(std::strerror(errno)));
    }
    process_utils::SignalInfo sig_info;
    process_utils::LoadSignalInfo(status, &sig_info);
    return sig_info;
  }

  pid_t Tracer::LaunchTargetProcess() {
    return proc_launcher->Spawn(&tracee);
  }

  void Tracer::ProcessCrash(process_utils::SignalInfo sig_info) {
    DEBUG_PRINT("PROCESSING CRASH\n");
    last_crash.crashed = true;
    last_crash.signal_info = sig_info;

    const ser::AsanInfo* asan_info = tracee.GetAsanInfo();

    // simple case: we used ASAN and just need to copy the info
    if (asan_info != nullptr) {
      last_crash.major_stack.clear();
      last_crash.minor_stack.clear();
      memcpy(last_crash.major_hash, asan_info->major_hash, sizeof(asan_info->major_hash));
      memcpy(last_crash.minor_hash, asan_info->minor_hash, sizeof(asan_info->minor_hash));


    // more complicated: we didn't use ASAN and we need to calculate / collect the info manually
    } else {
      last_crash.major_hash[0] = 0;
      last_crash.minor_hash[0] = 0;

      CalcHashes();

      if (last_crash.major_hash[0] == 0) {
        Rand rand;
        std::string charset = "abcdefghijklmnopqrstuvwxyz";

        snprintf(last_crash.major_hash, sizeof(last_crash.major_hash), "%s", "unknown_exit");
        last_crash.major_stack = "unknown_exit";
        last_crash.minor_stack = "unknown_exit";
        std::string out;
        resmack::utils::RandBytes(&rand, charset.c_str(), 10, &out);
        snprintf(last_crash.minor_hash, sizeof(last_crash.minor_hash), "%s", out.c_str());
      }
    }
  }

  void Tracer::MonitorTracee() {
    while (should_run.load()) {
      traced_pid.store(LaunchTargetProcess());
      process_utils::SignalInfo exit_info = WaitForExit();

      if (!should_run.load()) {
        break;
      }

      switch(exit_info.exit_reason) {
        case process_utils::ExitReason::Normal:
          DEBUG_PRINT("MonitorTracee:: Normal exit\n");
          continue;
        case process_utils::ExitReason::Crash:
          DEBUG_PRINT("MonitorTracee:: Crashing exit\n");
          ProcessCrash(exit_info);
          exception_cb(traced_pid.load(), this, &tracee);

          // this happens if we aren't using ASAN
          if (exit_info.stopped && !exit_info.exited) {
            pid_t pid = traced_pid.load();
            kill(pid, SIGKILL);

            int reap_status;
            // don't leave it hanging (reap it)
            waitpid(pid, &reap_status, 0);
          }

          break;
        case process_utils::ExitReason::Timeout:
          DEBUG_PRINT("MonitorTracee:: Timeout exit\n");
          timeout_cb(traced_pid.load(), this, &tracee);
          break;
        default:
          break;
      }
    }
  }

  // https://github.com/daniel-thompson/libunwind-examples/blob/master/unwind-pid.c
  void Tracer::CalcHashes() {
    last_crash.major_stack.clear();
    last_crash.minor_stack.clear();

    unw_addr_space_t as = unw_create_addr_space(&_UPT_accessors, 0);
    void *context = _UPT_create(traced_pid.load());
    unw_cursor_t cursor;
    int res = unw_init_remote(&cursor, as, context);
    if (res != 0) {
      _UPT_destroy(context);
      unw_destroy_addr_space(as);
      throw std::runtime_error(std::format("Could not create stack-unwinding cursor: {}", res));
    }

    // last five frames
    std::string* major_stack = &last_crash.major_stack;
    // all frames
    std::string* minor_stack = &last_crash.minor_stack;

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

    utils::sha1_hex(major_stack->data(), major_stack->size(), last_crash.major_hash);
    utils::sha1_hex(minor_stack->data(), minor_stack->size(), last_crash.minor_hash);
  }

}
}
