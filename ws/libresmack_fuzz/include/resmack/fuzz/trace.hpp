#ifndef RESMACK_FUZZ_TRACE_H
#define RESMACK_FUZZ_TRACE_H

#include <string>
#include <algorithm>
#include <pthread.h>
#include <sys/ptrace.h>

#include "resmack/rand.hpp"
#include "resmack/types.hpp"
#include "resmack/fuzz/serialized.hpp"
#include "resmack/fuzz/tracee.hpp"
#include "resmack/fuzz/trace_target.hpp"

namespace resmack {
namespace fuzz {

class Tracer;

// Intended to operate on the status variable (switch statement, etc). Returning
// true means that everything has handled. False means that the process should
// be terminated and restarted.
//
// The callback is also allowed to call Tracer::Stop to stop the loop
//
// See 'man waitpid` and the wstatus explanation as well as the 'man ptrace'
// explanation for the PTRACE_SET_OPTIONS for ways to determine the cause of
// the signal. E.g.
//
//   status>>8 == (SIGTRAP | (PTRACE_EVENT_FORK<<8))
//
using TraceExceptionCb =
  std::function<bool(pid_t pid, int status, Tracer*, Tracee*)>;

struct CrashInfo {
  int signal;
};

class Tracer {
 private:
  Tracee tracee;
  TraceTarget* target;
  pid_t traced_pid;
  bool should_run;
  TraceExceptionCb exception_cb;
  CrashInfo last_crash;

  pthread_t monitor_thread;

 public:
  Tracer(TraceTarget* target, TraceExceptionCb cb);
  ~Tracer();

  // target does whatever it needs to do to return a pid_t and set whether
  // it should be attached or if it's good to go. The target is run in a
  // separate thread, calling the call back when an exception has occurred.
  //
  // **NOTE** The callback must be thread-safe!
  void Trace();
  void Stop();
  void Join();
  CrashInfo* GetCrashInfo() { return &this->last_crash; }
 
  static void* MonitorTracee(void* this_arg);
};

/*
struct Registers {
  size_t rip;
  size_t rsp;
  size_t rbp;
  size_t rax;
  size_t rbx;
  size_t rcx;
  size_t rdx;
  size_t rsi;
  size_t rdi;
  size_t r8;
  size_t r9;
  size_t r10;
  size_t r11;
  size_t r12;
  size_t r13;
  size_t r14;
  size_t r15;
};

class Crash {
 public:
  Registers registers;
  int signal;
  size_t hash_major;
  size_t hash_minor;
  Vector<std::string> backtrace;

  static void TraceMe();

  Crash(Tracer* tracer);
  ~Crash();
};
*/

}
}

#endif
