#include <signal.h>
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

}
}
}
