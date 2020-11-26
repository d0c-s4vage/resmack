#ifndef RESMACK_FUZZ_TRACE_FORK_H
#define RESMACK_FUZZ_TRACE_FORK_H

#include "resmack/fuzz/trace_target.hpp"

namespace resmack {
namespace fuzz {
namespace trace_targets {

using TraceSpawnCb = std::function<void(Tracee*)>;

class Fork : public TraceTarget {
 private:
  TraceSpawnCb cb_;

 public:
  Fork(TraceSpawnCb spawn_cb);
  ~Fork();

  pid_t Spawn(Tracee* tracee);
};

}
}
}

#endif
