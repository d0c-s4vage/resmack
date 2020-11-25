#ifndef RESMACK_FUZZ_TRACE_TARGET_H_
#define RESMACK_FUZZ_TRACE_TARGET_H_

#include "sys/ptrace.h"

#include <functional>
#include <utility>

#include "resmack/fuzz/trace.hpp"

namespace resmack {
namespace fuzz {

using TraceSpawnCb = std::function<void()>;

class TraceTarget {
 private:
  TraceSpawnCb spawn_cb;

 public:
  TraceTarget(TraceSpawnCb spawn_cb);
  ~TraceTarget();

  virtual void InitTrace();
  virtual void Spawn(Tracee* tracee) = 0;
};

}
}

#endif
