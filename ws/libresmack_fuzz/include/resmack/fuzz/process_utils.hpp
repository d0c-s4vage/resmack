#ifndef RESMACK_FUZZ_PROCESS_UTILS_H
#define RESMACK_FUZZ_PROCESS_UTILS_H

namespace resmack {
namespace fuzz {
namespace process_utils {

  struct SignalInfo {
    bool stopped;
    bool exited;
    bool signaled;
    bool exit_status;
    bool stop_signal;
    bool term_signal;
  };

  void LoadSignalInfo(int status, SignalInfo* out);

}
}
}

#endif
