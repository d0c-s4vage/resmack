#include "gtest/gtest.h"

#include "resmack/rand.hpp"

namespace resmack {

  TEST(Rand, Seedable) {
    Rand rand1(100);
    Rand rand2(100);

    for (size_t i = 0; i < 100; i++) {
      EXPECT_EQ(rand1.Next(), rand2.Next());
    }
  }

  TEST(Rand, DifferentSeedsDefault) {
    Rand rand1;
    Rand rand2;

    for (size_t i = 0; i < 100; i++) {
      EXPECT_NE(rand1.Next(), rand2.Next());
    }
  }

  TEST(Rand, NextInRangeGaussian) {
    Rand rand1;

    std::map<uint32_t, size_t> counts;
    for (int i = 0; i < 1000; i++) {
      uint32_t res = rand1.NextInRangeGaussian(0, 100);

      if (counts.contains(res)) {
        counts[res]++;
      } else {
        counts[res] = 1;
      }
    }

    /*
    size_t max = 0;
    for (auto pair: counts) {
      if (pair.second > max) {
        max = pair.second;
      }
    }

    double chars = 50;
    double char_increment = (double)max / chars;
    for (auto pair: counts) {
      printf("%4d - %-10lu", pair.first, pair.second);
      for (int i = 0; i < (pair.second / char_increment); i++) {
        printf("█");
      }
      printf("\n");
    }
    */
  }

}
