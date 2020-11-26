#ifndef RESMACK_FUZZ_TRACE_TARGET_H_
#define RESMACK_FUZZ_TRACE_TARGET_H_

#include "sys/ptrace.h"

#include <functional>
#include <utility>

namespace resmack {
namespace fuzz {

using TraceSpawnCb = std::function<void(Tracee*)>;

class TraceTarget {
 private:
  TraceSpawnCb spawn_cb;

 public:
  TraceTarget(TraceSpawnCb spawn_cb);
  ~TraceTarget();

  virtual pid_t Spawn(Tracee* tracee) = 0;
};

}
}

#endif
