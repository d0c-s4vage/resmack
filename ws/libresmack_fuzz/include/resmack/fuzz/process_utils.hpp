#ifndef RESMACK_FUZZ_PROCESS_UTILS_H
#define RESMACK_FUZZ_PROCESS_UTILS_H

namespace resmack {
namespace fuzz {
namespace process_utils {
  enum class ExitReason {
    // the program exited by itself
    Normal,
    Crash,
    Timeout,
    Ptrace,
  };

  struct SignalInfo {
    ExitReason exit_reason;
    bool stopped;
    bool exited;
    bool signaled;
    int exit_status;
    int stop_signal;
    int term_signal;
  };

  SignalInfo ExtractSignalInfo(int status);

  bool IgnoreBasicSignals() noexcept;

}
}
}

#endif
