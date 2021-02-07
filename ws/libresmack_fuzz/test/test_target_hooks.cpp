#include <chrono>
#include <sys/mman.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "gtest/gtest.h"

#include "resmack/fuzz/ipc/locked_shared_mem.hpp"
#include "resmack/fuzz/target_hooks.hpp"

namespace resmack {
namespace fuzz {

  TEST(TargetHooks, EventHandlersAreCalledInOrder) {
    ipc::LockedSharedMem shared_mem(0x100);
    uint32_t* pre_start  = shared_mem.GetNextPtrAfter<uint32_t>(0);
    uint32_t* post_start = shared_mem.GetNextPtrAfter<uint32_t>(sizeof(uint32_t));
    uint32_t* pre_test   = shared_mem.GetNextPtrAfter<uint32_t>(sizeof(uint32_t));
    uint32_t* post_test  = shared_mem.GetNextPtrAfter<uint32_t>(sizeof(uint32_t));
    uint32_t* pre_stop   = shared_mem.GetNextPtrAfter<uint32_t>(sizeof(uint32_t));
    uint32_t* post_stop  = shared_mem.GetNextPtrAfter<uint32_t>(sizeof(uint32_t));

    TargetHooks hooks;
    (&hooks)
      ->AddPreStart([&pre_start]()   { *pre_start  = 1; })
      ->AddPostStart([&post_start]() { *post_start = 2; })
      ->AddPreTest([&pre_test]()     { *pre_test   = 3; })
      ->AddPostTest([&post_test]()   { *post_test  = 4; })
      ->AddPreStop([&pre_stop]()     { *pre_stop   = 5; })
      ->AddPostStop([&post_stop]()   { *post_stop  = 6; });

    hooks.ExecPreStart();
    EXPECT_EQ(*pre_start,  1);
    EXPECT_EQ(*post_start, 0);
    EXPECT_EQ(*pre_test,   0);
    EXPECT_EQ(*post_test,  0);
    EXPECT_EQ(*pre_stop,   0);
    EXPECT_EQ(*post_stop,  0);

    hooks.ExecPostStart();
    EXPECT_EQ(*pre_start,  1);
    EXPECT_EQ(*post_start, 2);
    EXPECT_EQ(*pre_test,   0);
    EXPECT_EQ(*post_test,  0);
    EXPECT_EQ(*pre_stop,   0);
    EXPECT_EQ(*post_stop,  0);

    hooks.ExecPreTest();
    EXPECT_EQ(*pre_start,  1);
    EXPECT_EQ(*post_start, 2);
    EXPECT_EQ(*pre_test,   3);
    EXPECT_EQ(*post_test,  0);
    EXPECT_EQ(*pre_stop,   0);
    EXPECT_EQ(*post_stop,  0);

    hooks.ExecPostTest();
    EXPECT_EQ(*pre_start,  1);
    EXPECT_EQ(*post_start, 2);
    EXPECT_EQ(*pre_test,   3);
    EXPECT_EQ(*post_test,  4);
    EXPECT_EQ(*pre_stop,   0);
    EXPECT_EQ(*post_stop,  0);

    hooks.ExecPreStop();
    EXPECT_EQ(*pre_start,  1);
    EXPECT_EQ(*post_start, 2);
    EXPECT_EQ(*pre_test,   3);
    EXPECT_EQ(*post_test,  4);
    EXPECT_EQ(*pre_stop,   5);
    EXPECT_EQ(*post_stop,  0);

    hooks.ExecPostStop();
    EXPECT_EQ(*pre_start,  1);
    EXPECT_EQ(*post_start, 2);
    EXPECT_EQ(*pre_test,   3);
    EXPECT_EQ(*post_test,  4);
    EXPECT_EQ(*pre_stop,   5);
    EXPECT_EQ(*post_stop,  6);
  }

}
}
