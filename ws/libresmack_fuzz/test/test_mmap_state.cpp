#include "gtest/gtest.h"
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>
#include "sys/types.h"
#include "sys/stat.h"

#include "resmack/fuzz/states/mmap.hpp"

namespace resmack {
namespace fuzz {

  TEST(MMapState, SharableBetweenForks) {
    const char* test_state = "/tmp/test.state";
    struct stat info;
    if (stat(test_state, &info) == 0) {
      remove(test_state);
    }

    resmack::fuzz::states::MmapState state(test_state);

    uint32_t inc_to = 10;

    pid_t pid = fork();
    if (pid == 0) {
      for (uint32_t i = 0; i < inc_to; i++) {
        state.IncNumIterations();
      }
      std::exit(0);
    } else {
      int status;
      waitpid(pid, &status, 0);
    }

    EXPECT_EQ(state.GetNumIterations(), inc_to);
  }

}
}
