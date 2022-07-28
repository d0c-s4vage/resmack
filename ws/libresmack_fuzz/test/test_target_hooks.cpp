#include <chrono>
#include <sys/mman.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "gtest/gtest.h"

#include "resmack/fuzz/ipc/locked_shared_mem.hpp"
#include "resmack/fuzz/ipc/shared_mem_lock.hpp"
#include "resmack/fuzz/target_hooks.hpp"

namespace resmack {
namespace fuzz {

  TEST(TargetHooks, EventHandlersAreCalledInOrder) {
    ipc::QueuedSharedMem shared_mem(0x100);
    uint32_t* pre_start  = shared_mem.GetNextPtrFor<uint32_t>();
    uint32_t* post_start = shared_mem.GetNextPtrFor<uint32_t>();
    uint32_t* pre_test   = shared_mem.GetNextPtrFor<uint32_t>();
    uint32_t* post_test  = shared_mem.GetNextPtrFor<uint32_t>();
    uint32_t* pre_stop   = shared_mem.GetNextPtrFor<uint32_t>();
    uint32_t* post_stop  = shared_mem.GetNextPtrFor<uint32_t>();

    TargetHooks hooks;
    (&hooks)
      ->AddPreStart([&pre_start](ipc::QueuedSharedMem*) {
          *pre_start  = 1;
      })
      ->AddPostStart([&post_start](ipc::QueuedSharedMem*, pid_t, targets::Target*) {
        *post_start = 2;
      })
      ->AddPreTest([&pre_test](ipc::QueuedSharedMem*) {
        *pre_test   = 3;
      })
      ->AddPostTest([&post_test](ipc::QueuedSharedMem*) {
        *post_test  = 4;
      })
      ->AddPreStop([&pre_stop](ipc::QueuedSharedMem*, pid_t) {
        *pre_stop   = 5;
      })
      ->AddPostStop([&post_stop](ipc::QueuedSharedMem*, pid_t) {
        *post_stop  = 6; 
      });

    hooks.ExecPreStart(&shared_mem);
    EXPECT_EQ(*pre_start,  1);
    EXPECT_EQ(*post_start, 0);
    EXPECT_EQ(*pre_test,   0);
    EXPECT_EQ(*post_test,  0);
    EXPECT_EQ(*pre_stop,   0);
    EXPECT_EQ(*post_stop,  0);

    hooks.ExecPostStart(&shared_mem, -1, NULL);
    EXPECT_EQ(*pre_start,  1);
    EXPECT_EQ(*post_start, 2);
    EXPECT_EQ(*pre_test,   0);
    EXPECT_EQ(*post_test,  0);
    EXPECT_EQ(*pre_stop,   0);
    EXPECT_EQ(*post_stop,  0);

    hooks.ExecPreTest(&shared_mem);
    EXPECT_EQ(*pre_start,  1);
    EXPECT_EQ(*post_start, 2);
    EXPECT_EQ(*pre_test,   3);
    EXPECT_EQ(*post_test,  0);
    EXPECT_EQ(*pre_stop,   0);
    EXPECT_EQ(*post_stop,  0);

    hooks.ExecPostTest(&shared_mem);
    EXPECT_EQ(*pre_start,  1);
    EXPECT_EQ(*post_start, 2);
    EXPECT_EQ(*pre_test,   3);
    EXPECT_EQ(*post_test,  4);
    EXPECT_EQ(*pre_stop,   0);
    EXPECT_EQ(*post_stop,  0);

    hooks.ExecPreStop(&shared_mem, -1);
    EXPECT_EQ(*pre_start,  1);
    EXPECT_EQ(*post_start, 2);
    EXPECT_EQ(*pre_test,   3);
    EXPECT_EQ(*post_test,  4);
    EXPECT_EQ(*pre_stop,   5);
    EXPECT_EQ(*post_stop,  0);

    hooks.ExecPostStop(&shared_mem, -1);
    EXPECT_EQ(*pre_start,  1);
    EXPECT_EQ(*post_start, 2);
    EXPECT_EQ(*pre_test,   3);
    EXPECT_EQ(*post_test,  4);
    EXPECT_EQ(*pre_stop,   5);
    EXPECT_EQ(*post_stop,  6);
  }

  TEST(TargetHooks, IpcSetupFunctionality) {
    ipc::QueuedSharedMem shared_mem;
    TargetHooks hooks;

    uint32_t* mem1 = NULL;
    uint32_t* mem2 = NULL;

    (&hooks)
      ->AddIpcSize([]() -> size_t { return sizeof(uint32_t); })
      ->AddIpcInit([&mem1](ipc::QueuedSharedMem* mem) {
          mem1 = mem->GetNextPtrFor<uint32_t>();
      })

      ->AddIpcSize([]() -> size_t { return sizeof(uint32_t); })
      ->AddIpcInit([&mem2](ipc::QueuedSharedMem* mem) {
          mem2 = mem->GetNextPtrFor<uint32_t>();
      });

    shared_mem.Init(hooks.ExecAndSumIpcSize());
    hooks.ExecIpcInit(&shared_mem);

    EXPECT_TRUE(mem1 != NULL);
    EXPECT_TRUE(mem2 != NULL);

    pid_t forked = fork();
    if (forked == 0) {
      *mem1 = 100;
      *mem2 = 200;
      std::exit(0);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    waitpid(forked, NULL, 0);

    EXPECT_EQ(shared_mem.DataSize(), sizeof(uint32_t) * 2);
    EXPECT_EQ(*mem1, 100u);
    EXPECT_EQ(*mem2, 200u);
  }

}
}
