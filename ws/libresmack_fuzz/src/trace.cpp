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

#include "resmack/rand.hpp"
#include "resmack/utils.hpp"
#include "resmack/fuzz/trace.hpp"
#include "resmack/fuzz/utils.hpp"
#include "resmack/fuzz/ipc_util.hpp"

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
    if (this->traced_pid <= 0) {
      return;
    }
    ptrace(PTRACE_DETACH, this->traced_pid, NULL, NULL);
    kill(this->traced_pid, SIGINT);
    waitpid(this->traced_pid, NULL, 0);
    kill(this->traced_pid, SIGKILL);
    waitpid(this->traced_pid, NULL, 0);
    this->traced_pid = -1;
  }

  void Tracer::Join() {
    pthread_join(this->monitor_thread, NULL);
  }

  void* Tracer::MonitorTraceeTimeout(void* args_arg) {
    MonitorTimeoutArgs* args = (MonitorTimeoutArgs*)args_arg;
    args->timedout = false;

    timespec end;
    float start_f, end_f, span;
    pid_t killed_pid = -1;

    while (*args->should_run) {
      pid_t pid = args->pid;
      if (pid == killed_pid) {
        ; // noop
      } else if (pid > 0 && args->should_monitor_tracee) {
        start_f = args->tracee->GetIterStart();
        clock_gettime(CLOCK_MONOTONIC, &end);

        end_f = end.tv_sec + 1e-9 * end.tv_nsec;
        span = end_f - start_f;

        // we may have already signaled that it has been timedout
        if (!args->timedout && span > args->timeout) {
          args->timedout = true;
          kill(pid, SIGINT);
          killed_pid = pid;
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return NULL;
  }

  void* Tracer::MonitorTracee(void* this_arg) {
    Tracer* this_ = (Tracer*)this_arg;

    int status;

    MonitorTimeoutArgs timeout_args {
      .timeout = this_->timeout,
      .tracee = &this_->tracee,
      .should_run = &this_->should_run,
      .timedout = false,
      .pid = -1,
    };
    this_->traced_pid = this_->target->Spawn(&this_->tracee);

    pthread_create(
      &this_->monitor_timeout_thread,
      NULL,
      &MonitorTraceeTimeout,
      (void*)&timeout_args
    );

    while (this_->should_run) {
      this_->tracee.Reset();

      timeout_args.pid = this_->traced_pid;
      timeout_args.timedout = false;
      timeout_args.should_monitor_tracee = true;
      {
        waitpid(this_->traced_pid, &status, 0);
      }
      timeout_args.should_monitor_tracee = false;
      bool timedout = timeout_args.timedout;

      // these need to be done before the pthread_join() for some reason. UB?
      bool exited = WIFEXITED(status);
      bool stopped = WIFSTOPPED(status);
      bool signaled = WIFSIGNALED(status);

      int exit_status = WEXITSTATUS(status);
      int crash_sig = WSTOPSIG(status);
      int term_sig = WTERMSIG(status);

      if (crash_sig == SIGWINCH) {
        ptrace(PTRACE_CONT, this_->traced_pid, NULL, SIGWINCH);
        continue;
      }

      this_->last_crash.crashed = false;
      ser::AsanInfo* asan_info = this_->tracee.GetAsanInfo();

      bool should_continue;
      if (timedout) {
        DEBUG_PRINT("%d: [[Timedout: signal: %s, forwarding signal\n", this_->traced_pid, strsignal(crash_sig));
        // need to wait one more time since the first signal sent to the process
        // SIGINT is used to help the child proc cleanup before being completely
        // killed
        ptrace(PTRACE_CONT, this_->traced_pid, NULL, SIGINT);
        DEBUG_PRINT("%d: [[Waiting to completely exit\n", this_->traced_pid);
        waitpid(this_->traced_pid, &status, 0);
        DEBUG_PRINT("%d: [[Exited completely\n", this_->traced_pid);
        should_continue = this_->timeout_cb(this_->traced_pid, this_, &this_->tracee);
        DEBUG_PRINT("%d: [[Should continue: %d\n", this_->traced_pid, should_continue);
      } else {
        printf("%d: Dealing with non-timedout, exited program\n", this_->traced_pid);
        printf("%d: status: %d\n", this_->traced_pid, status);
        printf("%d: WIFEXITED: %d, WEXITSTATUS: %d\n", this_->traced_pid, exited, exit_status);
        printf("%d: WIFSTOPPED: %d, WSTOPSIG: %d - %s\n", this_->traced_pid, stopped, crash_sig, strsignal(crash_sig));
        printf("%d: WIFSIGNALED: %d, WTERMSIG: %d - %s\n", this_->traced_pid, signaled, term_sig, strsignal(term_sig));

        this_->last_crash.signal = crash_sig;
        if (asan_info != NULL) {
          this_->last_crash.crashed = true;
          printf("ASAN CRASH!!!\n");
          this_->last_crash.major_stack.clear();
          this_->last_crash.minor_stack.clear();
          memcpy(this_->last_crash.major_hash, asan_info->major_hash, sizeof(asan_info->major_hash));
          memcpy(this_->last_crash.minor_hash, asan_info->minor_hash, sizeof(asan_info->minor_hash));
          printf("  hashes - major: %s, minor: %s\n", this_->last_crash.major_hash, this_->last_crash.minor_hash);
        } else {
          this_->last_crash.major_hash[0] = 0;
          this_->last_crash.minor_hash[0] = 0;

          if (stopped) {
            printf("%d It stopped\n", this_->traced_pid);
            this_->last_crash.crashed = true;
            this_->CalcHashes();
            printf("  hashes - major: %s, minor: %s\n", this_->last_crash.major_hash, this_->last_crash.minor_hash);
          }

          if (this_->last_crash.major_hash[0] == 0) {
            printf("Unknown crash\n");
            Rand rand;
            std::string charset = "abcdefghijklmnopqrstuvwxyz";

            this_->last_crash.crashed = true;
            snprintf(this_->last_crash.major_hash, sizeof(this_->last_crash.major_hash), "%s", "unknown_exit");
            this_->last_crash.major_stack = "unknown_exit";
            this_->last_crash.minor_stack = "unknown_exit";
            std::string out;
            resmack::utils::RandBytes(&rand, charset.c_str(), charset.size(), 10, &out);
            snprintf(this_->last_crash.minor_hash, sizeof(this_->last_crash.minor_hash), "%s", out.c_str());
            printf("  hashes - major: %s, minor: %s\n", this_->last_crash.major_hash, this_->last_crash.minor_hash);
          }
        }

        should_continue = this_->exception_cb(this_->traced_pid, status, this_, &this_->tracee);
      }

      if (!should_continue) {
        printf("Told not to continue, breaking the loop and ending this fuzzing worker\n");
        break;
      }

      // default action is to kill the current process, and then restart it
      pid_t orig_pid = this_->traced_pid;
      kill(this_->traced_pid, SIGKILL);

      ptrace(PTRACE_DETACH, this_->traced_pid, NULL, NULL);
      this_->traced_pid = -1;

      this_->traced_pid = this_->target->Spawn(&this_->tracee);
    }

    printf("Exited MonitorTracee loop!\n");

    this_->Stop();
    pthread_join(this_->monitor_timeout_thread, NULL);

    return NULL;
  }

  // https://github.com/daniel-thompson/libunwind-examples/blob/master/unwind-pid.c
  void Tracer::CalcHashes() {
    printf("%d Calculating hashes 1\n", this->traced_pid);
    this->last_crash.major_stack.clear();
    this->last_crash.minor_stack.clear();

    unw_addr_space_t as = unw_create_addr_space(&_UPT_accessors, 0);

    void *context = _UPT_create(this->traced_pid);
    unw_cursor_t cursor;
    if (unw_init_remote(&cursor, as, context) != 0) {
      _UPT_destroy(context);
      free(as);
      printf("Could not init remote [libunwind]\n");
      return;
    }

    printf("%d Calculating hashes 13\n", this->traced_pid);
    // last five frames
    std::string* major_stack = &this->last_crash.major_stack;
    // all frames
    std::string* minor_stack = &this->last_crash.minor_stack;

    printf("%d Calculating hashes 13\n", this->traced_pid);
    char sym[4096];
    printf("%d Calculating hashes 14\n", this->traced_pid);
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
