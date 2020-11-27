#include <csignal>
#include <cstdlib>
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
  exception_cb(cb)
{}
Tracer::~Tracer() {}

void Tracer::Trace() {
  this->should_run = true;
  this->traced_pid = this->target->Spawn(&this->tracee);
  
  pthread_create(&this->monitor_thread, NULL, &MonitorTracee, (void*)this);
}

void Tracer::Stop() {
  this->should_run = false;
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
    waitpid(this_->traced_pid, &status, 0);

    if (WIFSTOPPED(status)) {
      this_->last_crash.signal = WSTOPSIG(status);
    }

    if (!this_->exception_cb(this_->traced_pid, status, this_, &this_->tracee)) {
      break;
    }
  }

  this_->Stop();
  pthread_exit(NULL);
  return NULL;
}

}
}
