#include <cstring>
#include <chrono>
#include <cxxabi.h>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <libunwind-ptrace.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <thread>
#include <time.h>
#include <unistd.h>

#include "resmack/fuzz/trace.hpp"
#include "resmack/fuzz/utils.hpp"

#include "asan_util.hpp"

namespace resmack {
namespace fuzz {

  Tracer::Tracer(
    TraceTarget* target,
    TraceExceptionCb cb,
    TraceTimeoutCb timeout_cb,
    uint32_t idx
  ) :
    tracee(idx),
    target(target),
    traced_pid(-1),
    timeout(5.0),
    exception_cb(cb),
    timeout_cb(timeout_cb)
  {}
  Tracer::~Tracer() {}

  void Tracer::Trace() {
    this->should_run = true;
    pthread_create(&this->monitor_thread, NULL, &MonitorTracee, (void*)this);
  }

  void Tracer::Stop() {
    this->should_run = false;
    ptrace(PTRACE_DETACH, this->traced_pid, NULL, NULL);
    if (this->traced_pid == -1) {
      return;
    }
    kill(this->traced_pid, SIGKILL);
  }

  void Tracer::Join() {
    void* rval;
    pthread_join(this->monitor_thread, &rval);
  }

  void* Tracer::MonitorTraceeTimeout(void* args_arg) {
    MonitorTimeoutArgs* args = (MonitorTimeoutArgs*)args_arg;
    pid_t pid = args->pid;

    timespec end;
    float start_f, end_f, span;
    bool timedout = false;

    while (args->should_monitor_tracee) {
      start_f = args->tracee->GetIterStart();
      clock_gettime(CLOCK_MONOTONIC, &end);

      end_f = end.tv_sec + 1e-9 * end.tv_nsec;
      span = end_f - start_f;

      //printf("%.04f-%.04f = %.04f, timeout: %.04f\n", end_f, start_f, span, args->timeout);
      if (span > args->timeout) {
        kill(pid, SIGKILL);
        timedout = true;
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    pthread_exit((void*)timedout);
  }

  void* Tracer::MonitorTracee(void* this_arg) {
    Tracer* this_ = (Tracer*)this_arg;

    int status;

    MonitorTimeoutArgs timeout_args {
      .timeout = this_->timeout,
      .tracee = &this_->tracee,
    };

    this_->traced_pid = this_->target->Spawn(&this_->tracee);
    while (this_->should_run) {
      this_->tracee.Reset();

      timeout_args.pid = this_->traced_pid;
      timeout_args.should_monitor_tracee = true;
      pthread_create(
        &this_->monitor_timeout_thread,
        NULL,
        &MonitorTraceeTimeout,
        (void*)&timeout_args
      );
      waitpid(this_->traced_pid, &status, 0);
      timeout_args.should_monitor_tracee = false;

      int crash_sig = WSTOPSIG(status);
      int exit_status = WEXITSTATUS(status);

      if (crash_sig == SIGWINCH) {
        ptrace(PTRACE_CONT, this_->traced_pid, NULL, SIGWINCH);
        continue;
      }

      bool timedout;
      pthread_join(this_->monitor_timeout_thread, (void**)&timedout);

      this_->last_crash.crashed = false;

      ser::AsanInfo* asan_info = this_->tracee.GetAsanInfo();

      // ignore resize signals
      if (WIFSTOPPED(status) && crash_sig != SIGKILL) {
        this_->last_crash.signal = crash_sig;
        // SIGILL -- illegal instruction
        // SIGSEGV - reading/writing outside of valid memory
        // SIGBUS - invalid pointer dereferenced
        //if (crash_sig == SIGILL || crash_sig == SIGSEGV || crash_sig == SIGBUS) {
          this_->last_crash.crashed = true;
          this_->CalcHashes();
        //}
      } else if (WIFEXITED(status)) {
        if (asan_info != NULL) {
          this_->last_crash.crashed = true;
          this_->last_crash.major_stack.clear();
          this_->last_crash.minor_stack.clear();
          memcpy(this_->last_crash.major_hash, asan_info->major_hash, sizeof(asan_info->major_hash));
          memcpy(this_->last_crash.minor_hash, asan_info->minor_hash, sizeof(asan_info->minor_hash));
        }/* else if (!timedout && exit_status == 0) {
          std::cout << "EXITED NORMALLY????" << std::endl;
          break;
        }
        */
      }

      if (timedout) {
        bool should_continue = this_->timeout_cb(this_->traced_pid, this_, &this_->tracee);
        if (!should_continue) {
          break;
        }
      } else if (this_->last_crash.crashed) {
        bool should_continue = this_->exception_cb(this_->traced_pid, status, this_, &this_->tracee);
        this_->last_crash.crashed = false;
        if (!should_continue) {
          break;
        }
      }

      // default action is to kill the current process, and then restart it
      ptrace(PTRACE_DETACH, this_->traced_pid, NULL, NULL);
      kill(this_->traced_pid, SIGKILL);
      this_->traced_pid = this_->target->Spawn(&this_->tracee);
    }

    this_->Stop();
    pthread_exit(NULL);
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

      if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
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
