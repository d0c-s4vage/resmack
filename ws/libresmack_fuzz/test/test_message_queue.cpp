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
    size_t data_length;
    TestStruct* read_test;
    rc = queue.ReadFromChild(&read_type, &data_length, reinterpret_cast<void**>(&read_test));
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
      _exit(0);
    } else {
      uint16_t received_type;
      size_t data_length;
      TestStruct* to_receive;
      rc = queue.ReadFromChild(&received_type, &data_length, reinterpret_cast<void**>(&to_receive));

      EXPECT_EQ(received_type, type);
      EXPECT_EQ(to_receive->a, to_send.a);
      EXPECT_EQ(to_receive->b, to_send.b);

      free(to_receive);
    }
  }

  TEST(MessageQueue, Performance) {
    bool rc;

    struct TestStruct {
      uint32_t message_number;
      uint64_t data[0x100];
    };

    MessageQueue queue;
    uint16_t type = 40;
    int iters = 0x10000;

    int pid = fork();
    if (pid == 0) {
      TestStruct to_send;
      for (int i = 0; i < iters; i++) {
        to_send.message_number = i;
        queue.SendToParent(type, &to_send);
      }
      _exit(0);
    }

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    TestStruct* to_receive;
    for (int i = 0; i < iters; i++) {
      uint16_t received_type;
      size_t data_length;
      rc = queue.ReadFromChild(&received_type, &data_length, reinterpret_cast<void**>(&to_receive));
      EXPECT_EQ(to_receive->message_number, i);
      free(to_receive);
      to_receive = nullptr;
    }

    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    printf("%0.03f message queue messages/s\n", (double)iters / span.count());

    waitpid(pid, NULL, 0);
  }

}
}
}
