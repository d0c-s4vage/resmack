#ifndef RESMACK_FUZZ_TRACE_FORK_H
#define RESMACK_FUZZ_TRACE_FORK_H

#include <functional>

#include "resmack/fuzz/process_launcher.hpp"

namespace resmack {
namespace fuzz {
namespace process_launchers {

  using TraceSpawnCb = std::function<void(Tracee*)>;

  class ForkLauncher : public ProcessLauncher {
   private:
    TraceSpawnCb cb;
    bool mute_io;

   public:
    ForkLauncher(bool mute_io, TraceSpawnCb spawn_cb);
    ~ForkLauncher();

    static void* SpawnThreadTarget(void* tracee_arg);
    pid_t Spawn(Tracee* tracee);
  };

  struct SpawnThreadArgs {
    ForkLauncher* this_;
    Tracee* tracee;
  };
}
}
}

#endif
