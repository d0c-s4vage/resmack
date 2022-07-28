#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#include "resmack/fuzz/process_utils.hpp"

namespace resmack {
namespace fuzz {
namespace process_utils {

  void LoadSignalInfo(int status, SignalInfo* out) {
    out->exited = WIFEXITED(status);
    out->stopped = WIFSTOPPED(status);
    out->signaled = WIFSIGNALED(status);

    out->exit_status = WEXITSTATUS(status);
    out->stop_signal = WSTOPSIG(status);
    out->term_signal = WTERMSIG(status);
  }

  void PrintSignalInfo(SignalInfo* info) {
    if (info->exited) {
      printf("SIG: Exited, status: %d\n", info->exit_status);
    } else if (info->stopped) {
      printf(
        "SIG: Stopped, signal: %d - %s\n",
        info->stop_signal,
        strsignal(info->stop_signal)
      );
    } else if (info->signaled) {
      printf(
        "SIG: Signaled, signal: %d - %s\n",
        info->term_signal,
        strsignal(info->term_signal)
      );
    }
  }

}
}
}
