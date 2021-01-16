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

  struct MonitorTimeoutArgs {
    pid_t pid;
    float timeout;
    Tracee* tracee;
    bool should_monitor_tracee;
  };

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
  using TraceTimeoutCb =
    std::function<bool(pid_t pid, Tracer*, Tracee*)>;

  struct CrashInfo {
    bool crashed;
    int signal;
    std::string major_stack;
    std::string minor_stack;
    // last 5 frames
    char major_hash[41];
    // all frames
    char minor_hash[41];
    Vector<std::string> stack_trace;
  };

  class Tracer {
   private:
    uint32_t idx;
    Tracee tracee;
    TraceTarget* target;
    pid_t traced_pid;
    bool should_run;
    float timeout;
    TraceExceptionCb exception_cb;
    TraceTimeoutCb timeout_cb;
    CrashInfo last_crash;

    pthread_t monitor_thread;
    pthread_t monitor_timeout_thread;

   public:
    Tracer(
      TraceTarget* target,
      TraceExceptionCb cb,
      TraceTimeoutCb timeout_cb,
      uint32_t idx
    );
    ~Tracer();

    uint32_t GetIdx() { return this->idx; }
    void Trace();
    void Stop();
    void Join();
    CrashInfo* GetCrashInfo() { return &this->last_crash; }
   
    static void* MonitorTracee(void* this_arg);
    static void* MonitorTraceeTimeout(void* this_arg);
   
   private:
    void CalcHashes();
  };

}
}

#endif
