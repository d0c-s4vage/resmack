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
#include "resmack/fuzz/ipc_util.hpp"
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
    this->should_run.store(true);
    pthread_create(&this->monitor_thread, NULL, &Tracer::DoMonitorTracee, (void*)this);
  }

  void Tracer::Stop(bool hard_stop) {
    this->should_run.store(false);

    if (hard_stop) {
      pid_t pid = this->traced_pid.load();
      if (pid > 0) {
        DEBUG_PRINT("KILLING THE TRACED PID: %d\n", pid);
        kill(pid, SIGKILL);
      }
    }
  }

  void Tracer::Join() {
    pthread_join(this->monitor_thread, NULL);
    pthread_join(this->monitor_timeout_thread, NULL);
  }

  void* Tracer::DoMonitorTraceeTimeout(void* this_arg) {
    static_cast<Tracer*>(this_arg)->MonitorTraceeTimeout();
    return NULL;
  }

  void* Tracer::DoMonitorTracee(void* this_arg) {
    static_cast<Tracer*>(this_arg)->MonitorTracee();
    return NULL;
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

    while (this->should_run.load()) {
      {
        std::scoped_lock _lock(this->trace_lock);
        pid_t traced_pid = this->traced_pid.load();
        // hasn't started yet
        if (this->tracee.GetIterStart() <= 0.0f) {
          ; // noop
        } else {
          float now = utils::GetTimeNow();
          float iter_span = now  - this->tracee.GetIterStart();
          float lifetime_span = now - this->tracee.GetLifetimeStart();

          this->tracee.iter_timed_out = (iter_span >= this->timeout);
          this->tracee.lifetime_timed_out = (lifetime_span >= alive_max);
          if (this->tracee.iter_timed_out || this->tracee.lifetime_timed_out) {
            DEBUG_PRINT("KILLING THE TRACED PID (TIMEDOUT): %d\n", traced_pid);
            kill(traced_pid, SIGKILL);
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
    if (waitpid(this->traced_pid.load(), &status, 0) == -1) {
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

  void Tracer::InitTimeoutMonitor() {
    int res = pthread_create(
      &this->monitor_timeout_thread,
      NULL,
      &Tracer::DoMonitorTraceeTimeout,
      (void*)this
    );
    if (res != 0) {
      throw std::runtime_error("Could not create timeout monitoring thread: " + std::string(std::strerror(errno)));
    }
  }

  pid_t Tracer::LaunchTargetProcess() {
    return this->proc_launcher->Spawn(&this->tracee);
  }

  void Tracer::ProcessCrash(process_utils::SignalInfo sig_info) {
    DEBUG_PRINT("PROCESSING CRASH\n");
    this->last_crash.crashed = true;
    this->last_crash.signal_info = sig_info;

    const ser::AsanInfo* asan_info = this->tracee.GetAsanInfo();

    // simple case: we used ASAN and just need to copy the info
    if (asan_info != NULL) {
      this->last_crash.major_stack.clear();
      this->last_crash.minor_stack.clear();
      memcpy(this->last_crash.major_hash, asan_info->major_hash, sizeof(asan_info->major_hash));
      memcpy(this->last_crash.minor_hash, asan_info->minor_hash, sizeof(asan_info->minor_hash));


    // more complicated: we didn't use ASAN and we need to calculate / collect the info manually
    } else {
      this->last_crash.major_hash[0] = 0;
      this->last_crash.minor_hash[0] = 0;

      this->CalcHashes();

      if (this->last_crash.major_hash[0] == 0) {
        Rand rand;
        std::string charset = "abcdefghijklmnopqrstuvwxyz";

        snprintf(this->last_crash.major_hash, sizeof(this->last_crash.major_hash), "%s", "unknown_exit");
        this->last_crash.major_stack = "unknown_exit";
        this->last_crash.minor_stack = "unknown_exit";
        std::string out;
        resmack::utils::RandBytes(&rand, charset.c_str(), charset.size(), 10, &out);
        snprintf(this->last_crash.minor_hash, sizeof(this->last_crash.minor_hash), "%s", out.c_str());
      }
    }
  }

  void Tracer::MonitorTracee() {
    this->InitTimeoutMonitor();

    while (this->should_run.load()) {
      this->traced_pid.store(this->LaunchTargetProcess());
      process_utils::SignalInfo exit_info = this->WaitForExit();

      if (resmack::fuzz::ipc_util::SHUTTING_DOWN.load()) {
        break;
      }

      switch(exit_info.exit_reason) {
        case process_utils::ExitReason::Normal:
          DEBUG_PRINT("MonitorTracee:: Normal exit\n");
          continue;
        case process_utils::ExitReason::Crash:
          DEBUG_PRINT("MonitorTracee:: Crashing exit\n");
          this->ProcessCrash(exit_info);
          this->exception_cb(this->traced_pid.load(), this, &this->tracee);
          break;
        case process_utils::ExitReason::Timeout:
          DEBUG_PRINT("MonitorTracee:: Timeout exit\n");
          this->timeout_cb(this->traced_pid.load(), this, &this->tracee);
          break;
        default:
          break;
      }
    }
  }

  // https://github.com/daniel-thompson/libunwind-examples/blob/master/unwind-pid.c
  void Tracer::CalcHashes() {
    this->last_crash.major_stack.clear();
    this->last_crash.minor_stack.clear();

    unw_addr_space_t as = unw_create_addr_space(&_UPT_accessors, 0);
    void *context = _UPT_create(this->traced_pid.load());
    unw_cursor_t cursor;
    int res = unw_init_remote(&cursor, as, context);
    if (res != 0) {
      _UPT_destroy(context);
      unw_destroy_addr_space(as);
      throw std::runtime_error(std::format("Could not create stack-unwinding cursor: {}", res));
    }

    // last five frames
    std::string* major_stack = &this->last_crash.major_stack;
    // all frames
    std::string* minor_stack = &this->last_crash.minor_stack;

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
        char* demangled = abi::__cxa_demangle(sym, NULL, &demangled_size, &status);
        if (demangled != NULL) {
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

      if (strstr(sym, "LLVMFuzzerTestOneInput") != NULL) {
        break;
      }
    } while (unw_step(&cursor) > 0);

    _UPT_destroy(context);
    unw_destroy_addr_space(as);

    utils::sha1_hex(major_stack->data(), major_stack->size(), this->last_crash.major_hash);
    utils::sha1_hex(minor_stack->data(), minor_stack->size(), this->last_crash.minor_hash);
  }

}
}
