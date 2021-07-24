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
  using TargetHookSingleIpcMemCb = std::function<void(ipc::QueuedSharedMem* mem)>;
  using TargetHookIpcMemCb = std::function<void(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem)>;
  using TargetHookIpcMemPidCb =
    std::function<void(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem, pid_t)>;
  using TargetHookIpcMemPidTargetCb =
    std::function<void(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem, pid_t, targets::Target*)>;

  class TargetHooks {
   private:
    Vector<TargetHookSizedCb> private_ipc_size;
    Vector<TargetHookSizedCb> global_ipc_size;
    Vector<TargetHookSingleIpcMemCb> private_ipc_init;
    Vector<TargetHookSingleIpcMemCb> global_ipc_init;

    Vector<TargetHookIpcMemCb> pre_start;
    Vector<TargetHookIpcMemCb> pre_start_in_target;
    Vector<TargetHookIpcMemPidTargetCb> post_start;

    Vector<TargetHookIpcMemCb> pre_test;
    Vector<TargetHookIpcMemCb> post_test;

    Vector<TargetHookIpcMemCb> on_crash;

    Vector<TargetHookIpcMemPidCb> pre_stop;
    Vector<TargetHookIpcMemPidCb> post_stop;

   public:
    TargetHooks();

    TargetHooks* AddPrivateIpcSize(TargetHookSizedCb call_back);
    TargetHooks* AddGlobalIpcSize(TargetHookSizedCb call_back);

    TargetHooks* AddPrivateIpcInit(TargetHookSingleIpcMemCb call_back);
    TargetHooks* AddGlobalIpcInit(TargetHookSingleIpcMemCb call_back);

    TargetHooks* AddPreStart(TargetHookIpcMemCb call_back);
    TargetHooks* AddPreStartInTarget(TargetHookIpcMemCb call_back);
    TargetHooks *AddPostStart(TargetHookIpcMemPidTargetCb call_back);

    TargetHooks* AddPreTest(TargetHookIpcMemCb call_back);
    TargetHooks* AddPostTest(TargetHookIpcMemCb call_back);

    TargetHooks* AddOnCrash(TargetHookIpcMemCb callback);

    TargetHooks* AddPreStop(TargetHookIpcMemPidCb call_back);
    TargetHooks* AddPostStop(TargetHookIpcMemPidCb call_back);

    size_t ExecAndSumPrivateIpcSize();
    size_t ExecAndSumGlobalIpcSize();

    void ExecPrivateIpcInit(ipc::QueuedSharedMem* private_mem);
    void ExecGlobalIpcInit(ipc::QueuedSharedMem* global_mem);

    void ExecPreStart(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem);
    void ExecPreStartInTarget(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem);
    void ExecPostStart(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem, pid_t pid, targets::Target* target);

    // don't need the pid for both of these since they run *IN* the
    // target process - can simply do getpid() if it's needed
    void ExecPreTest(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem);
    void ExecPostTest(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem);

    void ExecOnCrash(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem);

    void ExecPreStop(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem, pid_t pid);
    void ExecPostStop(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem, pid_t pid);

   private:
    size_t ExecAndSumSizeCallbacks(Vector<TargetHookSizedCb>* callbacks);
  };

}
}

#endif
