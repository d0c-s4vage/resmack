#ifndef RESMACK_FUZZ_TARGET_NEW_DIRECT_H
#define RESMACK_FUZZ_TARGET_NEW_DIRECT_H

#include <functional>
#include <string>

#include "resmack/fuzz/target_new.hpp"
#include "resmack/fuzz/target_hooks.hpp"
#include "resmack/fuzz/ipc/locked_shared_mem.hpp"
#include "resmack/fuzz/ipc/shared_mem_condition.hpp"

namespace resmack {
namespace fuzz {
namespace targets {

  using TargetCb = std::function<int(const char* data, size_t size)>;

  class DirectTarget : Target {
   private:
    int kReadyVal = 0x11223344;

    pid_t running_target;
    TargetCb callback;
    ipc::LockedSharedMem ipc_memory;
    TargetHooks hooks;

    ipc::SharedMemCondition* input_ready;
    ipc::SharedMemCondition* input_processed;
    size_t* ipc_data_size;
    char* ipc_data;
    int* ipc_result;

    void TestLoop();

   public:
    DirectTarget(TargetCb callback, TargetHooks hooks, size_t max_input_size);
    ~DirectTarget();

    pid_t Start();
    void Stop();
    int Test(const std::string* input);
  };

}
}
}

#endif
