#ifndef RESMACK_FUZZ_TARGET_NEW_DIRECT_H
#define RESMACK_FUZZ_TARGET_NEW_DIRECT_H

#include <functional>
#include <string>

#include "resmack/fuzz/generator.hpp"
#include "resmack/fuzz/target_new.hpp"
#include "resmack/fuzz/target_hooks.hpp"
#include "resmack/fuzz/ipc/locked_shared_mem.hpp"
#include "resmack/fuzz/ipc/shared_mem_condition.hpp"

namespace resmack {
namespace fuzz {
namespace targets {

  using TargetCb = std::function<int(const char* data, size_t size)>;

  class DirectTarget : public Target {
   private:
    size_t id;
    size_t max_input_size;
    pid_t running_target;
    TargetCb callback;
    ipc::QueuedSharedMem private_mem;
    ipc::QueuedSharedMem global_mem;
    TargetHooks* hooks;
    Generator* genr;

    void TestLoop();
    bool InitPrivateMem();

   public:
    DirectTarget(size_t id, TargetCb callback, TargetHooks* hooks, size_t max_input_size, Generator* genr);
    ~DirectTarget();

    pid_t Start();
    void Stop();
    ipc::QueuedSharedMem* GetPrivateMem() { return &this->private_mem; }
    ipc::QueuedSharedMem* GetGlobalMem() { return &this->global_mem; }
  };

}
}
}

#endif
