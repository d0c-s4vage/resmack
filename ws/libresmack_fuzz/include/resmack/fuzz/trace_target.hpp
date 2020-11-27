#ifndef RESMACK_FUZZ_TRACE_TARGET_H_
#define RESMACK_FUZZ_TRACE_TARGET_H_

#include "resmack/fuzz/tracee.hpp"

namespace resmack {
namespace fuzz {

class TraceTarget {
 public:
  virtual pid_t Spawn(Tracee* tracee) = 0;
};

}
}

#endif
