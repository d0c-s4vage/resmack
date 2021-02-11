#include <chrono>

#include "gtest/gtest.h"

#include "resmack/fuzz/ipc/locked_shared_mem.hpp"
#include "resmack/fuzz/targets_new/direct.hpp"
#include "resmack/fuzz/target_hooks.hpp"

namespace resmack {
namespace fuzz {
namespace targets {

  struct IpcInfo {
    bool pre_started;
    uint32_t test_count;
  };

  TEST(NewDirectTarget, StartsAndStops) {
    TargetHooks hooks;

    IpcInfo* info;

    (&hooks)
      ->AddIpcSize([]() -> size_t { return sizeof(bool) + sizeof(IpcInfo); })
      ->AddIpcInit([&info](ipc::LockedSharedMem* mem) {
        info = mem->GetNextPtrFor<IpcInfo>();
        info->test_count = 0;
      })
      ->AddPreStartInTarget([&info](ipc::LockedSharedMem* mem) {
          info->pre_started = true;
      })
      ;

    auto cb = [&info](const char* data, size_t) -> int {
      info->test_count += 1;
      return info->test_count;
    };

    std::string input = "input_data";
    DirectTarget target(cb, hooks, input.size() + 1);

    target.Start();

    int res;

    //std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    int iters = 100;
    for (int i = 0; i < iters; i++) {
      res = target.Test(&input);
      EXPECT_EQ(res, i+1);
    }
    target.Stop();
    //std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    //std::chrono::duration<double> span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    //printf("%0.03f iters/s\n", (double)iters / span.count());

    EXPECT_EQ(res, iters);
    EXPECT_EQ(info->test_count, iters);
    target.Stop();
  }

}
}
}
