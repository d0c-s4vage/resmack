#ifndef RESMACK_FUZZ_TRACE_TARGET_H_
#define RESMACK_FUZZ_TRACE_TARGET_H_

#include "sys/ptrace.h"

#include <functional>
#include <utility>

namespace resmack {
namespace fuzz {

class TraceTarget {
 public:
  virtual pid_t Spawn(Tracee* tracee) = 0;
};

}
}

#endif
