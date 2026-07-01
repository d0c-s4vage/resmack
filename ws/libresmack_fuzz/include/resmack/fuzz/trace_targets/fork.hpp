#ifndef RESMACK_FUZZ_TRACE_FORK_H
#define RESMACK_FUZZ_TRACE_FORK_H

#include <functional>

#include "resmack/fuzz/asan_util.hpp"
#include "resmack/fuzz/trace_target.hpp"

namespace resmack {
namespace fuzz {
namespace trace_targets {

  using TraceSpawnCb = std::function<void(Tracee*)>;

  class Fork : public TraceTarget {
   private:
    TraceSpawnCb cb;
    bool mute_io;

   public:
    Fork(bool mute_io, TraceSpawnCb spawn_cb);
    ~Fork();

    ATTRIBUTE_NO_SANITIZING
    static void* SpawnThreadTarget(void* tracee_arg);

    ATTRIBUTE_NO_SANITIZING
    pid_t Spawn(Tracee* tracee);
  };

  struct SpawnThreadArgs {
    Fork* this_;
    Tracee* tracee;
  };
}
}
}

#endif
