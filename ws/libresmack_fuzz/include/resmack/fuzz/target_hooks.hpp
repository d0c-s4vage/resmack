#ifndef RESMACK_FUZZ_TARGET_HOOKS_H
#define RESMACK_FUZZ_TARGET_HOOKS_H

#include <functional>

#include "resmack/fuzz/ipc/queued_shared_mem.hpp"
#include "resmack/types.hpp"

namespace resmack {
namespace fuzz {

  namespace targets {
    class Target;
  }

  using TargetHookGenericCb = std::function<void()>;
  using TargetHookSizedCb = std::function<size_t()>;
  using TargetHookPidCb = std::function<void(pid_t)>;
  using TargetHookPidCb = std::function<void(pid_t)>;
  using TargetHookIpcMemCb = std::function<void(ipc::QueuedSharedMem*)>;
  using TargetHookIpcMemPidCb =
    std::function<void(ipc::QueuedSharedMem *, pid_t)>;
  using TargetHookIpcMemPidTargetCb =
    std::function<void(ipc::QueuedSharedMem *, pid_t, targets::Target*)>;

  class TargetHooks {
   private:
    Vector<TargetHookSizedCb> ipc_size;
    Vector<TargetHookIpcMemCb> ipc_init;

    Vector<TargetHookIpcMemCb> pre_start;
    Vector<TargetHookIpcMemCb> pre_start_in_target;
    Vector<TargetHookIpcMemPidTargetCb> post_start;

    Vector<TargetHookIpcMemCb> pre_test;
    Vector<TargetHookIpcMemCb> post_test;

    Vector<TargetHookIpcMemPidCb> pre_stop;
    Vector<TargetHookIpcMemPidCb> post_stop;

   public:
    TargetHooks();

    TargetHooks* AddIpcSize(TargetHookSizedCb call_back);
    TargetHooks* AddIpcInit(TargetHookIpcMemCb call_back);

    TargetHooks* AddPreStart(TargetHookIpcMemCb call_back);
    TargetHooks* AddPreStartInTarget(TargetHookIpcMemCb call_back);
    TargetHooks *AddPostStart(TargetHookIpcMemPidTargetCb call_back);

    TargetHooks* AddPreTest(TargetHookIpcMemCb call_back);
    TargetHooks* AddPostTest(TargetHookIpcMemCb call_back);

    TargetHooks* AddPreStop(TargetHookIpcMemPidCb call_back);
    TargetHooks* AddPostStop(TargetHookIpcMemPidCb call_back);

    size_t ExecAndSumIpcSize();
    void ExecIpcInit(ipc::QueuedSharedMem* ipc_mem);

    void ExecPreStart(ipc::QueuedSharedMem* ipc_mem);
    void ExecPreStartInTarget(ipc::QueuedSharedMem* ipc_mem);
    void ExecPostStart(ipc::QueuedSharedMem* ipc_mem, pid_t pid, targets::Target* target);

    // don't need the pid for both of these since they run *IN* the
    // target process - can simply do getpid() if it's needed
    void ExecPreTest(ipc::QueuedSharedMem* ipc_mem);
    void ExecPostTest(ipc::QueuedSharedMem* ipc_mem);

    void ExecPreStop(ipc::QueuedSharedMem* ipc_mem, pid_t pid);
    void ExecPostStop(ipc::QueuedSharedMem* ipc_mem, pid_t pid);
  };

}
}

#endif
