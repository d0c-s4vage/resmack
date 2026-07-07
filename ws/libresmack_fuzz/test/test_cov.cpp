#include <cstdio>

#include "gtest/gtest.h"

#include "resmack/debug.hpp"
#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/feedbacks/coverage.hpp"

namespace resmack {
namespace fuzz {

  __attribute__((no_sanitize("coverage")))
  void _TestCoverageHitsTwice() {
    uint32_t vars[3] = {0, 0, 0};
    HandleSanitizerCovTracePcGuardInit(&vars[0], &vars[2]);

    Coverage cov;
    cov.Start();

    HandleSanitizerCovTracePcGuard(&vars[0]);
    HandleSanitizerCovTracePcGuard(&vars[1]);

    cov.Stop();

    EXPECT_NE(cov.GetStats().key, 0);
  }

  TEST(Fuzz, CoverageHitTwice) {
    _TestCoverageHitsTwice();
  }

  __attribute__((no_sanitize("coverage")))
  void _TestCoverageNotHit() {
    Coverage cov;
    cov.Start();
    cov.Stop();

    EXPECT_EQ(cov.GetStats().new_coverage, false);
  }

  TEST(Fuzz, CoverageNotHit) {
    _TestCoverageNotHit();
  }

}
}
