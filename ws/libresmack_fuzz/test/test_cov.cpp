#include <cstdio>

#include "gtest/gtest.h"

#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/feedbacks/coverage.hpp"

namespace resmack {
namespace fuzz {

  TEST(Fuzz, CoverageHitTwice) {
    uint32_t vars[3] = {0, 0, 0};
    __sanitizer_cov_trace_pc_guard_init(&vars[0], &vars[2]);

    Coverage cov;
    cov.Start();

    __sanitizer_cov_trace_pc_guard(&vars[0]);
    __sanitizer_cov_trace_pc_guard(&vars[1]);

    cov.Stop();

    EXPECT_NE(cov.GetStats().key, 0u);
  }

  TEST(Fuzz, CoverageNotHit) {
    Coverage cov;
    cov.Start();
    cov.Stop();

    EXPECT_EQ(cov.GetStats().key, 0u);
  }

}
}
