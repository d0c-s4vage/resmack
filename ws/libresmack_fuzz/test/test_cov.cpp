#include <cstdio>

#include "gtest/gtest.h"

#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/feedbacks/coverage.hpp"

namespace resmack {
namespace fuzz {
namespace feedbacks {

  TEST(Fuzz, CoverageHitTwice) {
    uint32_t vars[3] = {0, 0, 0};
    HandleSanitizerCovTracePcGuardInit(&vars[0], &vars[2]);

    Coverage cov;
    cov.Clear();
    cov.TestInitShared();

    HandleSanitizerCovTracePcGuard(&vars[0]);
    HandleSanitizerCovTracePcGuard(&vars[1]);
    cov.CalcHash();

    EXPECT_NE(cov.GetStats().key, 0u);

    cov.TestDestroyShared();
  }

  TEST(Fuzz, CoverageNotHit) {
    uint32_t vars[3] = {0, 0, 0};
    HandleSanitizerCovTracePcGuardInit(&vars[0], &vars[2]);

    Coverage cov;
    cov.TestInitShared();
    cov.Clear();
    cov.CalcHash();

    EXPECT_EQ(cov.GetStats().key, 0u);

    cov.TestDestroyShared();
  }

  TEST(Fuzz, CoverageWorksWithIPC) {
    uint32_t vars[3] = {0, 0, 0};
    HandleSanitizerCovTracePcGuardInit(&vars[0], &vars[2]);

    ipc::QueuedSharedMem mem(0x100);
    mem.ListenForUpdates();

    TargetHooks hooks;
    Coverage cov;
    cov.InsertHooks(&hooks);

    hooks.ExecAndSumIpcSize();
    hooks.ExecIpcInit(&mem);
    hooks.ExecPreStartInTarget(&mem);
    hooks.ExecPreTest(&mem);

    HandleSanitizerCovTracePcGuard(&vars[0]);
    HandleSanitizerCovTracePcGuard(&vars[1]);

    hooks.ExecPostTest(&mem);

    mem.StopListeningForUpdates();

    EXPECT_NE(cov.GetStats().key, 0u);
  }

}
}
}
