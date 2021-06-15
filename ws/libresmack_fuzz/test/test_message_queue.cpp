#include <chrono>
#include <sys/mman.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "gtest/gtest.h"

#include "resmack/fuzz/ipc/message_queue.hpp"

namespace resmack {
namespace fuzz {
namespace ipc {

  TEST(MessageQueue, BasicMessagePassingWorks) {
    bool rc;

    struct TestStruct {
      uint16_t a;
      uint16_t b;
    };
    TestStruct test;
    test.a = 0xaa00;
    test.b = 0xbb00;

    MessageQueue queue;
    rc = queue.SendToParent(1, &test);
    EXPECT_TRUE(rc);

    uint16_t read_type = 0;
    TestStruct* read_test;
    rc = queue.ReadFromChild(&read_type, &read_test);
    EXPECT_TRUE(rc);

    EXPECT_EQ(read_type, 1u);
    EXPECT_EQ(read_test->a, test.a);
    EXPECT_EQ(read_test->b, test.b);
    free(read_test);
  }

  TEST(MessageQueue, WorksWithFork) {
    bool rc;

    struct TestStruct {
      uint16_t a;
      uint16_t b;
    };

    TestStruct to_send { .a = 20, .b = 30 };

    MessageQueue queue;
    uint16_t type = 40;

    if (fork() == 0) {
      queue.SendToParent(type, &to_send);
    } else {
      uint16_t received_type;
      TestStruct* to_receive;
      rc = queue.ReadFromChild(&received_type, &to_receive);

      EXPECT_EQ(received_type, type);
      EXPECT_EQ(to_receive->a, to_send.a);
      EXPECT_EQ(to_receive->b, to_send.b);

      free(to_receive);
    }
  }

}
}
}
