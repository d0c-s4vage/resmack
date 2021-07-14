#ifndef RESMACK_FUZZ_TRACER_H
#define RESMACK_FUZZ_TRACER_H

#include <string>
#include <algorithm>
#include <libunwind-ptrace.h>
#include <pthread.h>
#include <sys/ptrace.h>

#include "resmack/types.hpp"
#include "resmack/fuzz/lock.hpp"
#include "resmack/fuzz/serialized.hpp"
#include "resmack/fuzz/target_hooks.hpp"
#include "resmack/fuzz/target_new.hpp"

namespace resmack {
namespace fuzz {

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

    bool* has_asan_info;
    char* asan_info;
    char* asan_major_stack;
    char* asan_minor_stack;
    char* asan_major_hash;
    char* asan_minor_hash;
  };

  class Tracer {
   private:
    CrashInfo last_crash;
    pthread_t monitor_thread;
    bool should_run;
    pid_t traced_pid;
    targets::Target* target;
    size_t max_asan_size;
    size_t max_stack_size;

    void CalcHashesLocal();
    void CalcHashesRemote();
    void CalcHashes(unw_cursor_t* cursor, const char* skip_until_past);

   public:
    Tracer();
    ~Tracer();

    void MonitorTracedPid();
    void InsertHooks(TargetHooks* hooks);
    bool DidCrash() { return this->last_crash.crashed; }
    CrashInfo* GetCrashInfo() { return &this->last_crash; }
    void WaitForEvent();
    void SaveAsanInfo(const char* report);
  };

}
}

#endif
