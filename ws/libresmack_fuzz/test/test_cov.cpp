#include <cstdio>

#include "gtest/gtest.h"

#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/feedbacks/coverage.hpp"

namespace resmack {
namespace fuzz {

  __attribute__((no_sanitize("coverage")))
  void TestCoverageHitsTwice() {
    uint32_t vars[3] = {0, 1, 2};
    HandleSanitizerCovTracePcGuardInit(&vars[0], &vars[2]);

    Coverage cov;
    cov.FPOStart();

    HandleSanitizerCovTracePcGuard(&vars[0]);
    HandleSanitizerCovTracePcGuard(&vars[1]);

    cov.FPOStop();
    FeedbackStats stats = cov.GetStats();

    EXPECT_EQ(stats.new_coverage, true);
    EXPECT_EQ(stats.num, 2);
    EXPECT_NE(stats.key, 1);
  }

  TEST(Fuzz, CoverageHitTwice) {
    TestCoverageHitsTwice();
  }

  __attribute__((no_sanitize("coverage")))
  void TestCoverageNotHit() {
    Coverage cov;
    cov.FPOStart();
    cov.FPOStop();

    EXPECT_EQ(cov.GetStats().new_coverage, false);
  }

  TEST(Fuzz, CoverageNotHit) {
    TestCoverageNotHit();
  }

}
}
