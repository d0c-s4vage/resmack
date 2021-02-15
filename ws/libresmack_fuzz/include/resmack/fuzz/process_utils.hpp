#ifndef RESMACK_FUZZ_PROCESS_UTILS_H
#define RESMACK_FUZZ_PROCESS_UTILS_H

namespace resmack {
namespace fuzz {
namespace process_utils {

  struct SignalInfo {
    bool stopped;
    bool exited;
    bool signaled;
    int exit_status;
    int stop_signal;
    int term_signal;
  };

  void LoadSignalInfo(int status, SignalInfo* out);

}
}
}

#endif
