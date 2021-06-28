#include <chrono>
#include <sys/mman.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "gtest/gtest.h"

#include "resmack/fuzz/ipc/queued_shared_mem.hpp"

namespace resmack {
namespace fuzz {
namespace ipc {

  struct DataUpdate {
    size_t number1;
    size_t number2;
    size_t number3;
    size_t number4;
  };

  TEST(QueuedSharedMem, BasicMessagePassingWorks) {
    QueuedSharedMem mem;
    mem.Init(0x1000);
    size_t* number1 = mem.GetNextPtrFor<size_t>();
    size_t* number2 = mem.GetNextPtrFor<size_t>();
    size_t* number3 = mem.GetNextPtrFor<size_t>();
    size_t* number4 = mem.GetNextPtrFor<size_t>();
    *number1 = 0;
    *number2 = 0;
    *number3 = 0;
    *number4 = 0;

    size_t iters = 0x10000;

    uint16_t update_type = 20;
    std::chrono::high_resolution_clock::time_point end;
    mem.AddReceiveHandler(update_type, [iters, &mem, &end, &number1, &number2, &number3, &number4](uint16_t, void* data, LockedSharedMem*) {
        DataUpdate* data_update = reinterpret_cast<DataUpdate*>(data);
        *number1 += data_update->number1;
        *number2 += data_update->number2;
        *number3 += data_update->number3;
        *number4 += data_update->number4;
        free(data_update);
        end = std::chrono::high_resolution_clock::now();
    });

    pid_t pid;
    if ((pid = fork()) == 0) {
      DataUpdate update;

      for (size_t i = 0; i < iters; i++ ){
        update.number1 = 1;
        update.number2 = 1;
        update.number3 = 1;
        update.number4 = 1;
        EXPECT_TRUE(mem.QueueUpdate(update_type, &update));
      }

      _exit(0);
    }

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    mem.ListenForUpdates();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    mem.StopListeningForUpdates();
    std::chrono::duration<double> span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    printf("%0.03f message queue messages/s\n", (double)iters / span.count());

    EXPECT_EQ(*number1, iters);
    EXPECT_EQ(*number2, iters);
    EXPECT_EQ(*number3, iters);
    EXPECT_EQ(*number4, iters);

    waitpid(pid, NULL, 0);
  }

}
}
}
