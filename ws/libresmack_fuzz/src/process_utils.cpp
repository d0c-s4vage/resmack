#include <cstdlib>
#include <csignal>
#include <cstring>
#include <signal.h>
#include <sys/wait.h>

#include "resmack/debug.hpp"
#include "resmack/fuzz/asan_util.hpp"
#include "resmack/fuzz/process_utils.hpp"

namespace resmack {
namespace fuzz {
namespace process_utils {

  static inline bool IsCrashSignal(int signal) {
    switch (signal) {
        case SIGTRAP:
          // a trace event
          return false;
        case SIGSEGV:  // segfault
        case SIGABRT:  // abort() / assert / ASAN
        case SIGBUS:   // misaligned memory access
        case SIGFPE:   // divide by zero / float exception
        case SIGILL:   // illegal instruction
        default:
            return true;
    }
  }

  void LoadSignalInfo(int status, SignalInfo* out) {
#if DEBUG_MESSAGES
    DEBUG_PRINT("SIGNAL_INFO: %d\n", status);
    DEBUG_PRINT("    WIFEXITED(status): %d\n", WIFEXITED(status));
    DEBUG_PRINT( \
        "    WEXITSTATUS(status): %d %s %s\n",
        WEXITSTATUS(status),
        sigabbrev_np(WEXITSTATUS(status)),
        strsignal(WEXITSTATUS(status))
    );
    DEBUG_PRINT("    WIFSTOPPED(status): %d\n", WIFSTOPPED(status));
    DEBUG_PRINT(\
        "    WSTOPSIG(status): %d %s %s\n",
        WSTOPSIG(status),
        sigabbrev_np(WSTOPSIG(status)),
        strsignal(WSTOPSIG(status))
    );
    DEBUG_PRINT("    WIFSIGNALED(status): %d\n", WIFSIGNALED(status));
    DEBUG_PRINT(
        "    WTERMSIG(status): %d %s %s\n",
        WTERMSIG(status),
        sigabbrev_np(WTERMSIG(status)),
        strsignal(WTERMSIG(status))
    );
    DEBUG_PRINT("    WCOREDUMP(status): %d\n", WCOREDUMP(status));
#endif
    out->exited = WIFEXITED(status);
    out->exit_status = WEXITSTATUS(status);

    out->signaled = WIFSIGNALED(status);
    out->term_signal = WTERMSIG(status);

    out->stopped = WIFSTOPPED(status);
    out->stop_signal = WSTOPSIG(status);

    if(
      (out->exited && out->exit_status == asan::ASAN_EXIT_CODE)
      || (out->stopped && IsCrashSignal(out->stop_signal))
      || (out->signaled && out->term_signal == SIGABRT)
    ) {
      out->exit_reason = ExitReason::Crash;
    } else {
      out->exit_reason = ExitReason::Normal;
    }
  }

}
}
}
