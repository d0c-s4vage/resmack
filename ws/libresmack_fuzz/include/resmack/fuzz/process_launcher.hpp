#ifndef RESMACK_FUZZ_TRACE_TARGET_H_
#define RESMACK_FUZZ_TRACE_TARGET_H_

#include "resmack/fuzz/tracee.hpp"

namespace resmack {
namespace fuzz {

class ProcessLauncher {
 public:
  virtual pid_t Spawn(Tracee* tracee) = 0;
};

}
}

#endif
