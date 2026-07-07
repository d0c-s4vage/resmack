#include <cstring>
#include <chrono>
#include <cxxabi.h>
#include <csignal>
#include <cstdlib>
#include <libunwind-ptrace.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <thread>
#include <time.h>
#include <unistd.h>

#include "resmack/debug.hpp"
#include "resmack/rand.hpp"
#include "resmack/utils.hpp"
#include "resmack/fuzz/trace.hpp"
#include "resmack/fuzz/utils.hpp"
#include "resmack/fuzz/process_utils.hpp"


namespace resmack {
namespace fuzz {
  Tracer::Tracer(
    TraceTarget* target,
    TraceExceptionCb cb,
    TraceTimeoutCb timeout_cb,
    uint32_t idx
  ) :
    idx(idx),
    tracee(idx),
    target(target),
    traced_pid(-1),
    timeout(5.0),
    exception_cb(cb),
    timeout_cb(timeout_cb),
    timeout_lock("TraceTimeoutLock", true)
  {}
  Tracer::~Tracer() {}

  void Tracer::Trace() {
    this->should_run.store(true);
    pthread_create(&this->monitor_thread, NULL, &MonitorTracee, (void*)this);
  }

  void Tracer::Stop() {
    this->should_run.store(false);
    pid_t pid = this->traced_pid;
    if (pid <= 0) {
      return;
    }

    ptrace(PTRACE_DETACH, pid, NULL, NULL);

    // WHY WHY WHY WHY???
    // 
    //this->timeout_lock.lock();
    pthread_join(this->monitor_timeout_thread, NULL);

    kill(pid, SIGKILL);
    waitpid(pid, NULL, 0);
  }

  void Tracer::Join() {
    pthread_join(this->monitor_thread, NULL);
  }

  void* Tracer::MonitorTraceeTimeout(void* args_arg) {
    MonitorTimeoutArgs* args = (MonitorTimeoutArgs*)args_arg;
    args->timedout = false;

    timespec end;
    float end_f, iter_span;
    pid_t killed_pid = -1;

    pid_t last_pid = -1;
    timespec tmp;
    clock_gettime(CLOCK_MONOTONIC, &tmp);
    float pid_alive_start = tmp.tv_sec + 1e-9 * tmp.tv_nsec;
    float alive_max = 600.0f;

    pid_t pid;
    float curr_iter_start;
    while (args->should_run->load()) {
      {
        std::scoped_lock _lock(*args->timeout_lock);
        pid = args->pid;
        curr_iter_start = args->tracee->GetIterStart();
      }
      float sleep_ms = 1000.0f;

      if (pid == killed_pid) {
        ; // noop
      } else if (pid > 0 && args->should_monitor_tracee->load() && curr_iter_start != -1.0f) {
        if (last_pid != pid) {
          last_pid = pid;
          clock_gettime(CLOCK_MONOTONIC, &tmp);
          pid_alive_start = tmp.tv_sec + 1e-9 * tmp.tv_nsec;
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        end_f = end.tv_sec + 1e-9 * end.tv_nsec;

        iter_span = end_f - curr_iter_start;
        float alive_span = end_f - pid_alive_start;

        // we may have already signaled that it has been timedout
        if (!args->timedout && (iter_span > args->timeout || alive_span > alive_max)) {
          {
            std::scoped_lock _lock(*args->timeout_lock);
            args->timedout = true;
            kill(pid, SIGKILL);
            killed_pid = pid;
          }
        }
      }

      auto ms = std::chrono::milliseconds((int)sleep_ms);
      std::this_thread::sleep_for(ms);
    }

    return NULL;
  }

  void* Tracer::MonitorTracee(void* this_arg) {
    Tracer* this_ = (Tracer*)this_arg;

    int status;
    process_utils::SignalInfo sig_info;

    std::atomic<bool> should_monitor_tracee(false);
    MonitorTimeoutArgs timeout_args {
      .pid = -1,
      .timeout = this_->timeout,
      .tracee = &this_->tracee,
      .should_monitor_tracee = &should_monitor_tracee,
      .should_run = &this_->should_run,
      .timedout = false,
      .idx = this_->idx,
      .timeout_lock = &this_->timeout_lock,
    };

    DEBUG_PRINT("Monitoring tracee\n");
    while (this_->should_run.load()) {
      DEBUG_PRINT("Loop\n");
      this_->traced_pid = -1;
      this_->tracee.Reset();
      this_->traced_pid = this_->target->Spawn(&this_->tracee);

      pid_t curr_pid = this_->traced_pid;
      DEBUG_PRINT("curr_pid: %d\n", curr_pid);

      {
        DEBUG_PRINT("Waiting for the timeout lock\n");
        std::scoped_lock _l(this_->timeout_lock);
        DEBUG_PRINT("Got the timeout lock\n");
        timeout_args.pid = this_->traced_pid;
        timeout_args.timedout = false;
        should_monitor_tracee.store(true);

        DEBUG_PRINT("Creating timeout thread\n");
        pthread_create(
          &this_->monitor_timeout_thread,
          NULL,
          &MonitorTraceeTimeout,
          (void*)&timeout_args
        );
        DEBUG_PRINT("Done creating timeout thread\n");
      }

      DEBUG_PRINT("Waiting for thread to stop\n");
      if (waitpid(curr_pid, &status, 0) == -1) {
        DEBUG_PRINT("Error waiting for spawned target\n");
        throw std::runtime_error("Error waiting for child: " + std::string(std::strerror(errno)));
      }
      DEBUG_PRINT("Target stopped\n");

      bool timedout;
      {
        std::scoped_lock _l(this_->timeout_lock);
        should_monitor_tracee.store(false);
        timedout = timeout_args.timedout;
      }

      process_utils::LoadSignalInfo(status, &sig_info);

      if (sig_info.stopped && sig_info.stop_signal == SIGWINCH) {
        ptrace(PTRACE_CONT, curr_pid, NULL, SIGWINCH);
        continue;
      }

      this_->last_crash.crashed = false;
      ser::AsanInfo* asan_info = this_->tracee.GetAsanInfo();

      bool should_continue = true;
      if (timedout) {
        // need to wait one more time since the first signal sent to the process
        // SIGINT is used to help the child proc cleanup before being completely
        // killed. Specifically used to make sure no semaphores are still held
        // before exiting
        DEBUG_PRINT("SENDING SIGINT to the process\n");
        ptrace(PTRACE_CONT, curr_pid, NULL, SIGINT);
        if (waitpid(curr_pid, &status, 0) == -1) {
          DEBUG_PRINT("Waiting on the process again\n");
          perror("Error waiting for child");
        }
        should_continue = this_->timeout_cb(curr_pid, this_, &this_->tracee);
      } else if (sig_info.exited && sig_info.exit_status == 0) {
        DEBUG_PRINT("Nah, it didn't crash\n");
        should_continue = true;
      } else {
        DEBUG_PRINT("Definitely crashed\n");
        this_->last_crash.crashed = true;

        this_->last_crash.exit_status = sig_info.final_signal;
        this_->last_crash.signal = sig_info.final_signal;

        if (asan_info != NULL) {
          this_->last_crash.major_stack.clear();
          this_->last_crash.minor_stack.clear();
          memcpy(this_->last_crash.major_hash, asan_info->major_hash, sizeof(asan_info->major_hash));
          memcpy(this_->last_crash.minor_hash, asan_info->minor_hash, sizeof(asan_info->minor_hash));
        } else {
          this_->last_crash.major_hash[0] = 0;
          this_->last_crash.minor_hash[0] = 0;

          if (sig_info.stopped) {
            this_->CalcHashes();
          }

          if (this_->last_crash.major_hash[0] == 0) {
            Rand rand;
            std::string charset = "abcdefghijklmnopqrstuvwxyz";

            snprintf(this_->last_crash.major_hash, sizeof(this_->last_crash.major_hash), "%s", "unknown_exit");
            this_->last_crash.major_stack = "unknown_exit";
            this_->last_crash.minor_stack = "unknown_exit";
            std::string out;
            resmack::utils::RandBytes(&rand, charset.c_str(), charset.size(), 10, &out);
            snprintf(this_->last_crash.minor_hash, sizeof(this_->last_crash.minor_hash), "%s", out.c_str());
          }
        }

        should_continue = this_->exception_cb(curr_pid, status, this_, &this_->tracee);
      }

      if (!should_continue) {
        break;
      }

      // default action is to kill the current process, and then restart it
      if (curr_pid > 0) {
        ptrace(PTRACE_DETACH, curr_pid, NULL, NULL);
        kill(curr_pid, SIGKILL);
        waitpid(curr_pid, &status, 0);
      }
      //printf("[[[%d:Traced(%d) NEW CHILD\n", this_->idx, this_->traced_pid);
    }

    this_->Stop();

    return NULL;
  }

  // https://github.com/daniel-thompson/libunwind-examples/blob/master/unwind-pid.c
  void Tracer::CalcHashes() {
    this->last_crash.major_stack.clear();
    this->last_crash.minor_stack.clear();

    unw_addr_space_t as = unw_create_addr_space(&_UPT_accessors, 0);

    void *context = _UPT_create(this->traced_pid);
    unw_cursor_t cursor;
    if (unw_init_remote(&cursor, as, context) != 0) {
      _UPT_destroy(context);
      free(as);
      return;
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

      if (unw_get_reg(&cursor, UNW_REG_IP, &pc)) {
        _UPT_destroy(context);
        return;
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
      if (count > 0) { *minor_stack += "\n"; }
      *minor_stack += sym;

      if (strstr(sym, "LLVMFuzzerTestOneInput") != NULL) {
        break;
      }
    } while (unw_step(&cursor) > 0);

    _UPT_destroy(context);
    free(as);

    utils::sha1_hex(major_stack->data(), major_stack->size(), this->last_crash.major_hash);
    utils::sha1_hex(minor_stack->data(), minor_stack->size(), this->last_crash.minor_hash);
  }

}
}
