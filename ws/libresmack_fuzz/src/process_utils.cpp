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
        case SIGSEGV:  // segfault
        case SIGABRT:  // abort() / assert / ASAN
        case SIGBUS:   // misaligned memory access
        case SIGFPE:   // divide by zero / float exception
        case SIGILL:   // illegal instruction
          return true;

        case SIGTRAP:
        default:
          return false;
    }
  }

  SignalInfo ExtractSignalInfo(int status) {
//     DEBUG_PRINT("SIGNAL_INFO: %d\n", status);
//     DEBUG_PRINT("    WIFEXITED(status): %d\n", WIFEXITED(status));
//     DEBUG_PRINT( \
//         "    WEXITSTATUS(status): %d %s %s\n",
//         WEXITSTATUS(status),
//         sigabbrev_np(WEXITSTATUS(status)),
//         strsignal(WEXITSTATUS(status))
//     );
//     DEBUG_PRINT("    WIFSTOPPED(status): %d\n", WIFSTOPPED(status));
//     DEBUG_PRINT(\
//         "    WSTOPSIG(status): %d %s %s\n",
//         WSTOPSIG(status),
//         sigabbrev_np(WSTOPSIG(status)),
//         strsignal(WSTOPSIG(status))
//     );
//     DEBUG_PRINT("    WIFSIGNALED(status): %d\n", WIFSIGNALED(status));
//     DEBUG_PRINT(
//         "    WTERMSIG(status): %d %s %s\n",
//         WTERMSIG(status),
//         sigabbrev_np(WTERMSIG(status)),
//         strsignal(WTERMSIG(status))
//     );
//     DEBUG_PRINT("    WCOREDUMP(status): %d\n", WCOREDUMP(status));

    SignalInfo sig_info;

    sig_info.exited = WIFEXITED(status);
    sig_info.exit_status = WEXITSTATUS(status);

    sig_info.signaled = WIFSIGNALED(status);
    sig_info.term_signal = WTERMSIG(status);

    sig_info.stopped = WIFSTOPPED(status);
    sig_info.stop_signal = WSTOPSIG(status);

    if(
      (sig_info.exited && sig_info.exit_status == asan::ASAN_EXIT_CODE)
      || (sig_info.stopped && IsCrashSignal(sig_info.stop_signal))
      || (sig_info.signaled && sig_info.term_signal == SIGABRT)
    ) {
      DEBUG_PRINT("ExitReason::Crash\n");
      sig_info.exit_reason = ExitReason::Crash;
    } else if (sig_info.stopped) {
      sig_info.exit_reason = ExitReason::Ptrace;
      DEBUG_PRINT("ExitReason::Ptrace\n");
    } else {
      sig_info.exit_reason = ExitReason::Normal;
      DEBUG_PRINT("ExitReason::Normal\n");
    }

    return sig_info;
  }

  bool IgnoreBasicSignals() noexcept {
    const auto signals = std::to_array({
      // is handled by the main process, which shuts everything down.
      SIGINT,
      // terminal resize
      SIGWINCH,
    });

    bool success = true;
    for (const auto& sig: signals) {
      // the main parent process should be handling the SIGINT
      if (std::signal(sig, SIG_IGN) == SIG_ERR) {
        DEBUG_PRINT("ForkLauncher child: error installing %s handler\n", sigabbrev_np(sig));
        success = false;
      }
    }

    return success;
  }

}
}
}
