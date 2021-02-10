#ifndef RESMACK_FUZZ_TARGET_HOOKS_H
#define RESMACK_FUZZ_TARGET_HOOKS_H

#include <functional>

#include "resmack/fuzz/ipc/locked_shared_mem.hpp"
#include "resmack/types.hpp"

namespace resmack {
namespace fuzz {

  using TargetHookGenericCb = std::function<void()>;
  using TargetHookSizedCb = std::function<size_t()>;
  using TargetHookPidCb = std::function<void(pid_t)>;
  using TargetHookIpcMemCb = std::function<void(ipc::LockedSharedMem*)>;
  using TargetHookIpcMemPidCb =
    std::function<void(ipc::LockedSharedMem *, pid_t)>;

  class TargetHooks {
   private:
    Vector<TargetHookSizedCb> ipc_size;
    Vector<TargetHookIpcMemCb> ipc_init;

    Vector<TargetHookIpcMemCb> pre_start;
    Vector<TargetHookIpcMemCb> pre_start_in_target;
    Vector<TargetHookIpcMemPidCb> post_start;

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
    TargetHooks *AddPostStart(TargetHookIpcMemPidCb call_back);

    TargetHooks* AddPreTest(TargetHookIpcMemCb call_back);
    TargetHooks* AddPostTest(TargetHookIpcMemCb call_back);

    TargetHooks* AddPreStop(TargetHookIpcMemPidCb call_back);
    TargetHooks* AddPostStop(TargetHookIpcMemPidCb call_back);

    size_t ExecAndSumIpcSize();
    void ExecIpcInit(ipc::LockedSharedMem* ipc_mem);

    void ExecPreStart(ipc::LockedSharedMem* ipc_mem);
    void ExecPreStartInTarget(ipc::LockedSharedMem* ipc_mem);
    void ExecPostStart(ipc::LockedSharedMem* ipc_mem, pid_t pid);

    // don't need the pid for both of these since they run *IN* the
    // target process - can simply do getpid() if it's needed
    void ExecPreTest(ipc::LockedSharedMem* ipc_mem);
    void ExecPostTest(ipc::LockedSharedMem* ipc_mem);

    void ExecPreStop(ipc::LockedSharedMem* ipc_mem, pid_t pid);
    void ExecPostStop(ipc::LockedSharedMem* ipc_mem, pid_t pid);
  };

}
}

#endif
