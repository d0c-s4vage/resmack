#ifndef RESMACK_FUZZ_TRACE_H
#define RESMACK_FUZZ_TRACE_H

#include <string>
#include <functional>
#include <pthread.h>
#include <sys/ptrace.h>

#include "resmack/fuzz/asan_util.hpp"
#include "resmack/fuzz/lock.hpp"
#include "resmack/fuzz/tracee.hpp"
#include "resmack/fuzz/trace_target.hpp"

namespace resmack {
namespace fuzz {

  struct MonitorTimeoutArgs {
    pid_t pid;
    float timeout;
    Tracee* tracee;
    bool should_monitor_tracee;
    bool* should_run;
    bool timedout;
    uint32_t idx;
    Lock* timeout_lock;
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
    int exit_status;
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
    Lock timeout_lock;

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

    ATTRIBUTE_NO_SANITIZING
    void Trace();

    ATTRIBUTE_NO_SANITIZING
    void Stop();

    ATTRIBUTE_NO_SANITIZING
    void Join();

    const CrashInfo* GetCrashInfo() { return &this->last_crash; }
   
    ATTRIBUTE_NO_SANITIZING
    static void* MonitorTracee(void* this_arg);

    ATTRIBUTE_NO_SANITIZING
    static void* MonitorTraceeTimeout(void* this_arg);
   
   private:
    void CalcHashes();
  };

}
}

#endif
