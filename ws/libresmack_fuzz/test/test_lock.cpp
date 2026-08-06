#include <chrono>
#include <sys/mman.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "gtest/gtest.h"

#include "resmack/fuzz/lock.hpp"

namespace resmack {
namespace fuzz {

  TEST(Lock, WorksAcrossForks) {
    void* shared = mmap(
      nullptr,
      sizeof(uint32_t),
      PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANONYMOUS,
      -1,
      0
    );
    uint32_t* counter = (uint32_t*)shared;

    Lock lock("test-lock");
    lock.lock();

    pid_t forked;
    if ((forked = fork()) == 0) {
      lock.lock();
      *counter += 1;
      lock.unlock();
      std::exit(0);
    }

    int sleep_ms = 10;
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    // the increment shouldn't be happening until after the release
    *counter = 1;
    lock.unlock();

    waitpid(forked, nullptr, 0);
    EXPECT_EQ(*counter, 2);
  }

}
}
