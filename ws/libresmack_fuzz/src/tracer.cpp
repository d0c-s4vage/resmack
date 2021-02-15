#include <cstring>
#include <chrono>
#include <cxxabi.h>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <libunwind-ptrace.h>
#include <mutex>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <thread>
#include <time.h>
#include <unistd.h>

#include "resmack/fuzz/ipc/locked_shared_mem.hpp"
#include "resmack/fuzz/tracer.hpp"
#include "resmack/fuzz/target_hooks.hpp"
#include "resmack/fuzz/process_utils.hpp"

#include "asan_util.hpp"

namespace resmack {
namespace fuzz {

  Tracer::Tracer() : should_run(false), traced_pid(-1) {}
  Tracer::~Tracer() {}

  void Tracer::InsertHooks(TargetHooks* hooks) {
    size_t max_asan_size = 0x10000;
    hooks
      ->AddIpcSize([max_asan_size]() -> size_t { return max_asan_size; })
      ->AddIpcInit([this, max_asan_size](ipc::LockedSharedMem* mem) {
        char* ptr = mem->GetNextPtrFor<char>(max_asan_size);
        this->last_crash.asan_info = ptr;
        this->last_crash.asan_info[0] = '\0';
      })
      ->AddPreStartInTarget([this, max_asan_size](ipc::LockedSharedMem*) {
        printf("IN PRE START IN TARGET\n");
        printf("IN PRE START IN TARGET\n");
        printf("IN PRE START IN TARGET\n");
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        printf("Setting asan callback...\n");
        asan::SetAsanCallback([this, max_asan_size](const char* report) {
          size_t report_len = strlen(report);
          if (report_len > max_asan_size - 1) {
            report_len = max_asan_size - 1;
          }
          memcpy(this->last_crash.asan_info, report, report_len);
          this->last_crash.asan_info[report_len] = '\0';
        });
      })
      ->AddPostStart([this](ipc::LockedSharedMem*, pid_t new_pid) {
        this->traced_pid = new_pid;
        pthread_create(&this->monitor_thread, NULL, MonitorTracedPid, (void*)this);
      })
      ->AddPostStop([this](ipc::LockedSharedMem*, pid_t) {
        // force everything to wait until we've finished handling the
        // crash
        pthread_join(this->monitor_thread, NULL);
      });
  }

  void* Tracer::MonitorTracedPid(void* this_arg) {
    Tracer* this_ = reinterpret_cast<Tracer*>(this_arg);

    process_utils::SignalInfo sig_info;

    do {
      pid_t curr_pid = this_->traced_pid;

      int status;
      if (waitpid(curr_pid, &status, 0) == -1) {
        // EINTR == signal interrupted, ECHILD == pid does not exist
        if (errno == EINTR || errno == ECHILD) { break; }
        break;
      }

      printf("PROCESS CRASHED\n");
      printf("asan info len: %lu\n", strlen(this_->last_crash.asan_info));

      process_utils::LoadSignalInfo(status, &sig_info);
      // was intentionally killed, ignore it
      if (sig_info.stopped && sig_info.stop_signal == SIGKILL) {
        break;
      }

      // calc hash
      this_->CalcHashes();
    } while(false);

    return NULL;
  }

  /*
  void* Tracer::MonitorTracee(void* this_arg) {
    Tracer* this_ = (Tracer*)this_arg;

    int status;
    process_utils::SignalInfo sig_info;

    MonitorTimeoutArgs timeout_args {
      .timeout = this_->timeout,
      .tracee = &this_->tracee,
      .should_run = &this_->should_run,
      .timedout = false,
      .pid = -1,
      .idx = this_->idx,
      .timeout_lock = &this_->timeout_lock,
    };

    pthread_create(
      &this_->monitor_timeout_thread,
      NULL,
      &MonitorTraceeTimeout,
      (void*)&timeout_args
    );

    while (this_->should_run) {
      this_->traced_pid = -1;
      this_->tracee.Reset();
      this_->traced_pid = this_->target->Spawn(&this_->tracee);

      this_->timeout_lock.Acquire();
        pid_t curr_pid = this_->traced_pid;
        timeout_args.pid = this_->traced_pid;
        timeout_args.timedout = false;
        timeout_args.should_monitor_tracee = true;
      this_->timeout_lock.Release();

      if (waitpid(curr_pid, &status, 0) == -1) {
        perror("Error waiting for child");
      }

      this_->timeout_lock.Acquire();
        timeout_args.should_monitor_tracee = false;
        bool timedout = timeout_args.timedout;
      this_->timeout_lock.Release();

      process_utils::LoadSignalInfo(status, &sig_info);

      if (sig_info.stopped && sig_info.stop_signal == SIGWINCH) {
        ptrace(PTRACE_CONT, curr_pid, NULL, SIGWINCH);
        continue;
      }

      this_->last_crash.crashed = false;
      ser::AsanInfo* asan_info = this_->tracee.GetAsanInfo();

      bool should_continue;
      if (timedout) {
        // need to wait one more time since the first signal sent to the process
        // SIGINT is used to help the child proc cleanup before being completely
        // killed. Specifically used to make sure no semaphores are still held
        // before exiting
        ptrace(PTRACE_CONT, curr_pid, NULL, SIGINT);
        if (waitpid(curr_pid, &status, 0) == -1) {
          perror("Error waiting for child");
        }
        should_continue = this_->timeout_cb(curr_pid, this_, &this_->tracee);
      } else {
        this_->last_crash.crashed = true;
        if (sig_info.exited) {
          this_->last_crash.exit_status = sig_info.exit_status;
          this_->last_crash.signal = 0;
        } else if (sig_info.stopped) {
          this_->last_crash.exit_status = 0;
          this_->last_crash.signal = sig_info.stop_signal;
        } else if (sig_info.signaled) {
          this_->last_crash.exit_status = 0;
          this_->last_crash.signal = sig_info.term_signal;
        }

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
  */

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
