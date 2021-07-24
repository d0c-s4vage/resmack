#include <chrono>

#include "resmack/fuzz/stats.hpp"
#include "resmack/fuzz/ipc/queued_shared_mem.hpp"

namespace resmack {
namespace fuzz {

Stats::Stats() : ipc_stats(nullptr) {}

Stats::~Stats() {}

void Stats::InsertHooks(TargetHooks* hooks) {
  std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
  static uint64_t counter = 0;
  hooks
    ->AddGlobalIpcSize([]() -> size_t { return sizeof(StatsInfo); })
    ->AddGlobalIpcInit([this, start](ipc::QueuedSharedMem* mem) {
      this->ipc_stats = mem->GetNextPtrFor<StatsInfo>();
      memset(this->ipc_stats, 0, sizeof(StatsInfo));
      this->ipc_stats->iterations = 0;
      this->ipc_stats->crashes = 0;

      mem->AddReceiveHandler(UPDATE_TYPE, [this, start](size_t len, void* data, ipc::LockedSharedMem*) {
        StatsInfo* info = reinterpret_cast<StatsInfo*>(data);
        this->ipc_stats->crashes += info->crashes;
        this->ipc_stats->iterations += info->iterations;

        std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

        printf(
          "Total iters: %lu in %.03fs = %.03fs iters/s, %lu crashes\n",
          this->ipc_stats->iterations,
          span.count(),
          (double)this->ipc_stats->iterations / span.count(),
          this->ipc_stats->crashes
        );
      });
    })
    ->AddPreTest([](ipc::QueuedSharedMem*, ipc::QueuedSharedMem* global_mem) {
      counter++;
      if ((counter % 0x10000) != 0) {
        return;
      }

      StatsInfo info {
        .crashes = 0,
        .iterations = counter
      };
      counter = 0;
      global_mem->QueueUpdate(UPDATE_TYPE, &info);
    })
    ->AddOnCrash([](ipc::QueuedSharedMem*, ipc::QueuedSharedMem* global_mem) {
      StatsInfo info {
        .crashes = 1,
        .iterations = 1,
      };
      global_mem->QueueUpdate(UPDATE_TYPE, &info);
    });
}

}
}
