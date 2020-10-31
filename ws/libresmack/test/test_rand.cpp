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

}
