#include <csignal>
#include <cstdlib>
#include <iostream>
#include <pthread.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

#include "resmack/fuzz/trace.hpp"

namespace resmack {
namespace fuzz {

Tracer::Tracer(TraceTarget* target, TraceExceptionCb cb) :
  target(target),
  traced_pid(-1),
  exception_cb(cb),
  tracee()
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
    if (WIFSTOPPED(status)) {
      this_->last_crash.signal = crash_sig;
      // SIGILL -- illegal instruction
      // SIGSEGV - reading/writing outside of valid memory
      // SIGBUS - invalid pointer dereferenced
      this_->last_crash.crashed = (
        crash_sig == SIGILL || crash_sig == SIGSEGV || crash_sig == SIGBUS
      );
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
    this_->last_crash.crashed = false;
  }

  this_->Stop();
  pthread_exit(NULL);
  return NULL;
}

}
}
