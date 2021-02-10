#include <chrono>
#include <sys/mman.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "gtest/gtest.h"

#include "resmack/fuzz/ipc/locked_shared_mem.hpp"

namespace resmack {
namespace fuzz {
namespace ipc {

  TEST(LockedSharedMem, WorksAcrossForks) {
    LockedSharedMem shared_mem(0x100);
    EXPECT_EQ(shared_mem.DataSize(), 0x100u);

    uint32_t* counter1 = shared_mem.GetNextPtrFor<uint32_t>();
    uint32_t* counter2 = shared_mem.GetNextPtrFor<uint32_t>();
    shared_mem.Lock();

    pid_t fork_res = fork();
    EXPECT_NE(fork_res, -1);

    if (fork_res == 0) {
      shared_mem.Lock();
      *counter1 += 1;
      *counter2 += 1;
      shared_mem.Unlock();
      std::exit(0);
    }

    int sleep_ms = 10;
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    // the increments shouldn't be happening until after the release
    *counter1 = 10;
    *counter2 = 110;
    shared_mem.Unlock();

    waitpid(fork_res, NULL, 0);
    EXPECT_EQ(*counter1, 11);
    EXPECT_EQ(*counter2, 111);
  }

}
}
}
