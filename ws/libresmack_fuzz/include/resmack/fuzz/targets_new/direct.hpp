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

  struct DirectTargetIpcInfo {
    ipc::SharedMemCondition input_ready;
    ipc::SharedMemCondition input_processed;
    int result;
    size_t data_size;
    char data; // ref this to get a pointer to the data
  };

  class DirectTarget : public Target {
   private:
    size_t id;
    size_t max_input_size;
    pid_t running_target;
    TargetCb callback;
    ipc::LockedSharedMem ipc_memory;
    TargetHooks* hooks;

    DirectTargetIpcInfo* ipc;

    void TestLoop();

   public:
    DirectTarget(size_t id, TargetCb callback, TargetHooks* hooks, size_t max_input_size);
    ~DirectTarget();

    pid_t Start();
    void Stop();
    void ForceFinishTest();
    int Test(const std::string* input);
  };

}
}
}

#endif
