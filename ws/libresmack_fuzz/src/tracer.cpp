#include <cstring>
#include <chrono>
#include <cxxabi.h>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <libunwind.h>
#include <libunwind-x86_64.h>
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

#include "resmack/fuzz/debug.hpp"
#include "resmack/fuzz/ipc/locked_shared_mem.hpp"
#include "resmack/fuzz/tracer.hpp"
#include "resmack/fuzz/target_hooks.hpp"
#include "resmack/fuzz/process_utils.hpp"

#include "asan_util.hpp"

namespace resmack {
namespace fuzz {

  Tracer::Tracer() : should_run(false), traced_pid(-1), max_asan_size(0x10000), max_stack_size(0x1000) {}
  Tracer::~Tracer() {}

  void Tracer::InsertHooks(TargetHooks* hooks) {
    hooks
      ->AddPrivateIpcSize(
        [this]
        () -> size_t {
          return this->max_asan_size + (this->max_stack_size * 2) + (48 * 2) + 4 /*has_asan_info*/;
      })
      ->AddPrivateIpcInit(
        [this]
        (ipc::QueuedSharedMem* mem) {
          char* ptr = mem->GetNextPtrFor<char>(this->max_asan_size);
          this->last_crash.asan_info = ptr;
          this->last_crash.asan_info[0] = '\0';

          this->last_crash.asan_minor_stack = mem->GetNextPtrFor<char>(this->max_stack_size);
          memset(this->last_crash.asan_minor_stack, 0, this->max_stack_size);
          this->last_crash.asan_major_stack = mem->GetNextPtrFor<char>(this->max_stack_size);
          memset(this->last_crash.asan_major_stack, 0, this->max_stack_size);

          this->last_crash.asan_major_hash = mem->GetNextPtrFor<char>(48); // even boundaries
          memset(this->last_crash.asan_major_hash, 0, 0x48);
          this->last_crash.asan_minor_hash = mem->GetNextPtrFor<char>(48); // even boundaries
          memset(this->last_crash.asan_minor_hash, 0, 0x48);

          this->last_crash.has_asan_info = mem->GetNextPtrFor<bool>(4); // even boundaries
          *this->last_crash.has_asan_info = false;
      })
      ->AddPreStartInTarget(
        [this]
        (ipc::QueuedSharedMem*, ipc::QueuedSharedMem*) {
          ptrace(PTRACE_TRACEME, 0, NULL, NULL);

          asan::SetAsanCallback([this](const char* report) {
            this->SaveAsanInfo(report);
          });
      })
      ->AddPostStart(
        [this]
        (ipc::QueuedSharedMem*, ipc::QueuedSharedMem*, pid_t new_pid, auto* target) {
          this->traced_pid = new_pid;
          this->target = target;
          this->MonitorTracedPid();
          /*
          pthread_create(
            &this->monitor_thread,
            NULL,
            MonitorTracedPid,
            (void*)this);
            */
      })
      ->AddPostStop(
        [this]
        (ipc::QueuedSharedMem*, ipc::QueuedSharedMem*, pid_t) {
          // force everything to wait until we've finished handling the
          // crash
          //pthread_join(this->monitor_thread, NULL);

          if (*this->last_crash.has_asan_info) {
            this->last_crash.crashed = true;
            this->last_crash.minor_stack.assign(this->last_crash.asan_minor_stack);
            this->last_crash.major_stack.assign(this->last_crash.asan_major_stack);
            memcpy(this->last_crash.major_hash, this->last_crash.asan_major_hash, sizeof(this->last_crash.major_hash));
            memcpy(this->last_crash.minor_hash, this->last_crash.asan_minor_hash, sizeof(this->last_crash.minor_hash));
          }
      });
  }

  void Tracer::MonitorTracedPid() {
    process_utils::SignalInfo sig_info;

    do {
      pid_t curr_pid = this->traced_pid;
      this->last_crash.crashed = false;

      int status;
      if (waitpid(curr_pid, &status, 0) == -1) {
        // EINTR == signal interrupted, ECHILD == pid does not exist
        if (errno == EINTR || errno == ECHILD) {
          _DEBUG_PRINT("ERROR WAITING FOR CHILD %d: %d - %s\n", this->traced_pid, errno, strerror(errno));
          break;
        }
        break;
      }

      process_utils::LoadSignalInfo(status, &sig_info);

      if (sig_info.stopped) {
        if (sig_info.stop_signal == SIGWINCH) {
          ptrace(PTRACE_CONT, curr_pid, NULL, SIGWINCH);
          continue;
        }
        // CTRL-C
        else if (sig_info.stop_signal == SIGINT) {
          break;
        }
      } else if (sig_info.signaled && sig_info.term_signal == SIGKILL) {
        break;
      }

      this->last_crash.crashed = true;
      if (sig_info.stopped) {
        this->CalcHashesRemote();
      }
    } while(false);

    ptrace(PTRACE_CONT, this->traced_pid, NULL, NULL);
    ptrace(PTRACE_DETACH, this->traced_pid, NULL, NULL);

    this->target->Stop();
  }

  void Tracer::WaitForEvent() {
    pthread_join(this->monitor_thread, NULL);
  }

  void Tracer::SaveAsanInfo(const char* report) {
    size_t report_len = strlen(report);
    if (report_len > this->max_asan_size - 1) {
      report_len = this->max_asan_size - 1;
    }
    memcpy(this->last_crash.asan_info, report, report_len);
    this->last_crash.asan_info[report_len] = '\0';
    *this->last_crash.has_asan_info = true;

    this->CalcHashesLocal();

    memcpy(this->last_crash.asan_major_hash, this->last_crash.major_hash, sizeof(this->last_crash.major_hash));
    memcpy(this->last_crash.asan_minor_hash, this->last_crash.minor_hash, sizeof(this->last_crash.minor_hash));

    memcpy(
      this->last_crash.asan_major_stack,
      this->last_crash.major_stack.data(),
      this->last_crash.major_stack.length() > this->max_stack_size ? this->max_stack_size : this->last_crash.major_stack.length()
    );
    memcpy(
      this->last_crash.asan_minor_stack,
      this->last_crash.minor_stack.data(),
      this->last_crash.minor_stack.length() > this->max_stack_size ? this->max_stack_size : this->last_crash.minor_stack.length()
    );
  }

  void Tracer::CalcHashesLocal() {
    unw_cursor_t cursor;
    unw_context_t context;

    // grab the machine context and initialize the cursor
    if (unw_getcontext(&context) < 0) {
      std::cout << "ERROR: cannot get local machine state" << std::endl;
      std::exit(1);
    }
    if (unw_init_local(&cursor, &context) < 0) {
      std::cout << "ERROR: cannot initialize cursor for local unwinding" << std::endl;
      std::exit(1);
    }

    this->CalcHashes(&cursor, "__asan::Report");
  }

  void Tracer::CalcHashesRemote() {
    unw_addr_space_t as = unw_create_addr_space(&_UPT_accessors, 0);

    void* context = _UPT_create(this->traced_pid);
    unw_cursor_t cursor;
    int err;
    if ((err = unw_init_remote(&cursor, as, context)) != 0) {
      if (err == -UNW_EINVAL) {
        printf("UNW_EINVAL\n");
      } else if (err == -UNW_EUNSPEC) {
        printf("UNW_EUNSPEC\n");
      } else if (err == -UNW_EBADREG) {
        printf("could not init remote: UNW_EBADREG\n");
      }
      _UPT_destroy(context);
      free(as);
      return;
    }

    this->CalcHashes(&cursor, nullptr);

    _UPT_destroy(context);
    free(as);
  }

  // https://github.com/daniel-thompson/libunwind-examples/blob/master/unwind-pid.c
  void Tracer::CalcHashes(unw_cursor_t* cursor, const char* skip_until_past) {
    this->last_crash.major_stack.clear();
    this->last_crash.minor_stack.clear();

    // last five frames
    std::string* major_stack = &this->last_crash.major_stack;
    // all frames
    std::string* minor_stack = &this->last_crash.minor_stack;

    char sym[0x1000];
    size_t count = 0;
    // default case is that we already saw the skip_until_past and there's
    // nothing that needs to be skipped.
    bool saw_skip_until = (skip_until_past == nullptr);
    do {
      count++;
      unw_word_t offset;
      unw_word_t pc;

      if (unw_get_reg(cursor, UNW_REG_IP, &pc)) {
        return;
      }

      int res = unw_get_proc_name(cursor, sym, sizeof(sym), &offset);
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

      if (skip_until_past != nullptr) {
        // always skip the skip_until_past
        if (strstr(sym, skip_until_past) != nullptr) {
          saw_skip_until = true;
          continue;
        // keep skipping until we've seen the skip until past
        } else if (!saw_skip_until) {
          continue;
        }
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
    } while (unw_step(cursor) > 0);

    utils::sha1_hex(major_stack->data(), major_stack->size(), this->last_crash.major_hash);
    utils::sha1_hex(minor_stack->data(), minor_stack->size(), this->last_crash.minor_hash);
  }

}
}
