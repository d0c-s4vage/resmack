#include <cstring>
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
#include <unistd.h>

#include "resmack/fuzz/trace.hpp"
#include "resmack/fuzz/utils.hpp"

#include "asan_util.hpp"

namespace resmack {
namespace fuzz {

Tracer::Tracer(TraceTarget* target, TraceExceptionCb cb) :
  tracee(),
  target(target),
  traced_pid(-1),
  exception_cb(cb)
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

void* Tracer::MonitorTracee(void* this_arg) {
  Tracer* this_ = (Tracer*)this_arg;

  int status;

  this_->traced_pid = this_->target->Spawn(&this_->tracee);
  while (this_->should_run) {
    this_->tracee.Reset();
    waitpid(this_->traced_pid, &status, 0);

    int crash_sig = WSTOPSIG(status);
    int exit_status = WEXITSTATUS(status);

    std::cout << "WIFEXITED: " << WIFEXITED(status) << std::endl;
    if (WIFEXITED(status)) {
      std::cout << "  WEXITSTATUS: " << WEXITSTATUS(status) << std::endl;
    }

    std::cout << "WIFSIGNALED: " << WIFSIGNALED(status) << std::endl;
    if (WIFSIGNALED(status)) {
      std::cout << "  WTERMSIG: " << strsignal(WTERMSIG(status)) << std::endl;
      std::cout << "  WCOREDUMP: " << WCOREDUMP(status) << std::endl;
    }

    std::cout << "WIFSTOPPED: " << WIFSTOPPED(status) << std::endl;
    if (WIFSTOPPED(status)) {
      std::cout << "  WSTOPSIG: " << strsignal(WSTOPSIG(status)) << std::endl;
    }

    std::cout << "WIFCONTINUED: " << WIFCONTINUED(status) << std::endl;

    this_->last_crash.crashed = false;

    ser::AsanInfo* asan_info = this_->tracee.GetAsanInfo();
    if (WIFSTOPPED(status)) {
      this_->last_crash.signal = crash_sig;
      // SIGILL -- illegal instruction
      // SIGSEGV - reading/writing outside of valid memory
      // SIGBUS - invalid pointer dereferenced
      if (crash_sig == SIGILL || crash_sig == SIGSEGV || crash_sig == SIGBUS) {
        this_->last_crash.crashed = true;
        this_->CalcHashes();
      }
    } else if (asan_info != NULL) {
      this_->last_crash.crashed = true;
      this_->last_crash.major_stack.clear();
      this_->last_crash.minor_stack.clear();
      memcpy(this_->last_crash.major_hash, asan_info->major_hash, sizeof(asan_info->major_hash));
      memcpy(this_->last_crash.minor_hash, asan_info->minor_hash, sizeof(asan_info->minor_hash));
    }

    // exited normally - which should only occur if the FuzzLoop itself exited
    // normally. For example, when --max-iters is set and the maximum number
    // of iterations has been achieved
    if (WIFEXITED(status) && exit_status == 0) {
      break;
    }

    // ignore resize signals
    if (crash_sig == SIGWINCH) {
      ptrace(PTRACE_CONT, this_->traced_pid, NULL, SIGWINCH);
      continue;
    }

    if (this_->last_crash.crashed) {
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

  utils::sha1_hex(major_stack->data(), major_stack->size(), this->last_crash.major_hash);
  utils::sha1_hex(minor_stack->data(), minor_stack->size(), this->last_crash.minor_hash);
}

}
}
