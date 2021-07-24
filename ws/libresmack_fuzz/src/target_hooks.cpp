#include "resmack/fuzz/target_hooks.hpp"
#include "resmack/fuzz/ipc/locked_shared_mem.hpp"

namespace resmack {
namespace fuzz {

  TargetHooks::TargetHooks() {}

  TargetHooks* TargetHooks::AddPrivateIpcSize(TargetHookSizedCb callback) {
    this->private_ipc_size.push_back(callback);
    return this;
  }
  TargetHooks* TargetHooks::AddGlobalIpcSize(TargetHookSizedCb callback) {
    this->global_ipc_size.push_back(callback);
    return this;
  }
  TargetHooks* TargetHooks::AddPrivateIpcInit(TargetHookSingleIpcMemCb callback) {
    this->private_ipc_init.push_back(callback);
    return this;
  }
  TargetHooks* TargetHooks::AddGlobalIpcInit(TargetHookSingleIpcMemCb callback) {
    this->global_ipc_init.push_back(callback);
    return this;
  }

  TargetHooks* TargetHooks::AddPreStart(TargetHookIpcMemCb callback) {
    this->pre_start.push_back(callback);
    return this;
  }
  TargetHooks* TargetHooks::AddPreStartInTarget(TargetHookIpcMemCb callback) {
    this->pre_start_in_target.push_back(callback);
    return this;
  }
  TargetHooks* TargetHooks::AddPostStart(TargetHookIpcMemPidTargetCb callback) {
    this->post_start.push_back(callback);
    return this;
  }

  TargetHooks* TargetHooks::AddPreTest(TargetHookIpcMemCb callback) {
    this->pre_test.push_back(callback);
    return this;
  }
  TargetHooks* TargetHooks::AddPostTest(TargetHookIpcMemCb callback) {
    this->post_test.push_back(callback);
    return this;
  }

  TargetHooks* TargetHooks::AddOnCrash(TargetHookIpcMemCb callback) {
    this->on_crash.push_back(callback);
    return this;
  }

  TargetHooks* TargetHooks::AddPreStop(TargetHookIpcMemPidCb callback) {
    this->pre_stop.push_back(callback);
    return this;
  }
  TargetHooks* TargetHooks::AddPostStop(TargetHookIpcMemPidCb callback) {
    this->post_stop.push_back(callback);
    return this;
  }

  size_t TargetHooks::ExecAndSumPrivateIpcSize() {
    return this->ExecAndSumSizeCallbacks(&this->private_ipc_size);
  }

  size_t TargetHooks::ExecAndSumGlobalIpcSize() {
    return this->ExecAndSumSizeCallbacks(&this->global_ipc_size);
  }

  size_t TargetHooks::ExecAndSumSizeCallbacks(Vector<TargetHookSizedCb>* callbacks) {
    size_t res = 0;
    for (auto cb : *callbacks) {
      res += cb();
    }
    return res;
  }

  void TargetHooks::ExecPrivateIpcInit(ipc::QueuedSharedMem* ipc_mem) {
    for (auto cb : this->private_ipc_init) {
      cb(ipc_mem);
    }
  }

  void TargetHooks::ExecGlobalIpcInit(ipc::QueuedSharedMem* ipc_mem) {
    for (auto cb : this->global_ipc_init) {
      cb(ipc_mem);
    }
  }

  void TargetHooks::ExecPreStart(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem) {
    for (auto cb : this->pre_start) {
      cb(private_mem, global_mem);
    }
  }
  void TargetHooks::ExecPreStartInTarget(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem) {
    for (auto cb : this->pre_start_in_target) {
      cb(private_mem, global_mem);
    }
  }
  void TargetHooks::ExecPostStart(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem, pid_t pid, targets::Target* target) {
    for (auto cb : this->post_start) {
      cb(private_mem, global_mem, pid, target);
    }
  }

  void TargetHooks::ExecPreTest(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem) {
    for (auto cb : this->pre_test) {
      cb(private_mem, global_mem);
    }
  }
  void TargetHooks::ExecPostTest(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem) {
    for (auto cb : this->post_test) {
      cb(private_mem, global_mem);
    }
  }

  void TargetHooks::ExecOnCrash(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem) {
    for (auto cb : this->on_crash) {
      cb(private_mem, global_mem);
    }
  }

  void TargetHooks::ExecPreStop(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem, pid_t pid) {
    for (auto cb : this->pre_stop) {
      cb(private_mem, global_mem, pid);
    }
  }
  void TargetHooks::ExecPostStop(ipc::QueuedSharedMem* private_mem, ipc::QueuedSharedMem* global_mem, pid_t pid) {
    for (auto cb : this->post_stop) {
      cb(private_mem, global_mem, pid);
    }
  }

}
}
