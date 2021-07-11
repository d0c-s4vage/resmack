#ifndef RESMACK_FUZZ_STATS
#define RESMACK_FUZZ_STATS

#include <stddef.h>
#include <inttypes.h>

#include "resmack/fuzz/target_hooks.hpp"

namespace resmack {
namespace fuzz {


  struct StatsInfo {
    uint64_t crashes;
    uint64_t iterations;
  };

  class Stats {
   private:
    static const uint16_t UPDATE_TYPE = 0x12;
    StatsInfo* ipc_stats;

   public:
    Stats();
    ~Stats();
    void InsertHooks(TargetHooks* hooks);
    const StatsInfo* GetStats() { return this->ipc_stats; }
  };

}
}

#endif
