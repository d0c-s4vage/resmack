#include "gtest/gtest.h"
#include <cstddef>
#include <cstdio>
#include <string>

#include "resmack/fuzz/interface.hpp"
#include "resmack/fuzz/targets/direct.hpp"
#include "resmack/fuzz/feedbacks/coverage.hpp"

std::string TEST_INPUT;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t data_size) {
  TEST_INPUT.assign((char* )data, data_size);

  if (data_size < 4) {
    return 0;
  }

  // make sure it's longer than 0 clocks
  for (size_t i = 0; i < 0x1000000; i++) {
    if (data[0] == 'H') {
      if (data[1] == 'I') {
        if (data[2] == '!') {
          if (data[3] == '!') {
            return 0;
          }
        }
      }
    }
  }

  return 0;
}

namespace resmack {
namespace fuzz {

  TEST(TargetDirect, RunsCorrectly) {
    EXPECT_TRUE(false);
    /*
    DirectTarget target;
    Coverage feedback;
    TargetSettings settings;
    TargetStats stats(0x1);
    stats.stats_sample_interval = 1;

    std::string output = "Hello World";
    target.Launch(&feedback, &output, &settings, &stats);

    EXPECT_EQ(TEST_INPUT, output);
    EXPECT_NE(stats.duration_FEEDBACK, 0.0);
    EXPECT_NE(stats.duration_TARGET, 0.0);
    EXPECT_EQ(stats.duration_TARGET_RESET, 0.0);
    EXPECT_EQ(stats.crashed, false);
    */
  }

}
}
