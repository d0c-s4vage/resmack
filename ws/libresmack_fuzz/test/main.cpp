#include "gtest/gtest.h"

#include "resmack/fuzz/feedbacks/coverage.hpp"
#include "resmack/fuzz/sanitizer_init.hpp"
#include "resmack/fuzz/asan_util.hpp"

namespace resmack::fuzz {
  static bool SHUTTING_DOWN = false;
  INIT_SANITIZER_GUARDS(SHUTTING_DOWN);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
