#ifndef RESMACK_FUZZ_TRACE_FORK_H
#define RESMACK_FUZZ_TRACE_FORK_H

#include "resmack/fuzz/trace_target.hpp"

namespace resmack {
namespace fuzz {
namespace trace_targets {

class Fork : public TraceTarget {
 public:
   Fork(TraceSpawnCb spawn_cb);
   ~Fork();

  void Spawn(Tracee* tracee);
};

}
}
}

#endif
