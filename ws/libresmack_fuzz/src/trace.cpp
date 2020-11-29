#include <cstring>
#include <cxxabi.h>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <libunwind-ptrace.h>
#include <pthread.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <openssl/sha.h>

#include "resmack/fuzz/trace.hpp"

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
  kill(this->traced_pid, SIGKILL);
}

void Tracer::Join() {
  void* rval;
  pthread_join(this->monitor_thread, &rval);
}

void* Tracer::MonitorTracee(void* this_arg) {
  Tracer* this_ = (Tracer*)this_arg;

  int status;

  while (this_->should_run) {
    this_->traced_pid = this_->target->Spawn(&this_->tracee);
    waitpid(this_->traced_pid, &status, 0);

    int crash_sig = WSTOPSIG(status);

    this_->last_crash.crashed = false;
    if (WIFSTOPPED(status)) {
      this_->last_crash.signal = crash_sig;
      // SIGILL -- illegal instruction
      // SIGSEGV - reading/writing outside of valid memory
      // SIGBUS - invalid pointer dereferenced
      this_->last_crash.crashed = (
        crash_sig == SIGILL || crash_sig == SIGSEGV || crash_sig == SIGBUS
      );
      if (this_->last_crash.crashed) {
        this_->CalcHashes(
          &this_->last_crash.major_hash,
          &this_->last_crash.minor_hash
        );
      }
    }

    // exited normally - which should only occur if the FuzzLoop itself exited
    // normally. For example, when --max-iters is set and the maximum number
    // of iterations has been achieved
    if (WIFEXITED(status)) {
      break;
    }

    // ignore resize signals
    if (crash_sig == SIGWINCH) {
      ptrace(PTRACE_CONT, this_->traced_pid, NULL, NULL);
      continue;
    }

    if (crash_sig == SIGINT) {
      break;
    }

    if (!this_->exception_cb(this_->traced_pid, status, this_, &this_->tracee)) {
      break;
    }

    // default action is to kill the current process, and then restart it
    ptrace(PTRACE_DETACH, this_->traced_pid, NULL, NULL);
    kill(this_->traced_pid, SIGKILL);
  }

  this_->Stop();
  pthread_exit(NULL);
  return NULL;
}

// https://github.com/daniel-thompson/libunwind-examples/blob/master/unwind-pid.c
void Tracer::CalcHashes(size_t* major_hash, size_t* minor_hash) {
  unw_addr_space_t as = unw_create_addr_space(&_UPT_accessors, 0);

	void *context = _UPT_create(this->traced_pid);
	unw_cursor_t cursor;
	if (unw_init_remote(&cursor, as, context) != 0) {
    std::cout << "Couldn't initialize cursor for remote unwinding" << std::endl;
    _UPT_destroy(context);
    return;
  }

  std::string stack = "";

  char sym[4096];
	do {
		unw_word_t offset;
    unw_word_t pc;
    std::string frame_loc = "";

		if (unw_get_reg(&cursor, UNW_REG_IP, &pc)) {
      std::cout << "Could not read program counter" << std::endl;
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
      if (stack.size() > 0) { stack += "\n"; }
      stack += sym;

      if (strstr(sym, "LLVMFuzzerTestOneInput") != NULL) {
        break;
      }
    } else {
      stack += "??";
    }
	} while (unw_step(&cursor) > 0);

	_UPT_destroy(context);

  std::cout << "STACK:" << std::endl << stack << std::endl;
  unsigned char digest[SHA_DIGEST_LENGTH];
  SHA1((unsigned char *)stack.data(), stack.size(), (unsigned char *)digest);

  char hex_digest[(SHA_DIGEST_LENGTH * 2) + 1];
  for (size_t i = 0; i < SHA_DIGEST_LENGTH; i++) {
    snprintf(hex_digest + (i * 2), 4, "%02x", digest[i]);
  }
  std::cout << "DIGEST: " << hex_digest << std::endl;
}

}
}
