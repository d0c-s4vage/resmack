#include "gtest/gtest.h"

#include "resmack/fuzz/feedbacks/coverage.hpp"
#include "resmack/fuzz/sanitizer_init.hpp"
#include "resmack/fuzz/asan_util.hpp"

INIT_SANITIZER_GUARDS;

const char* __asan_default_options() {
  return resmack::fuzz::asan::ASAN_DEFAULT_OPTIONS;
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
