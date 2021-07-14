#include "resmack/fuzz/target_hooks.hpp"
#include "resmack/fuzz/ipc/locked_shared_mem.hpp"

namespace resmack {
namespace fuzz {

  TargetHooks::TargetHooks() {}

  TargetHooks* TargetHooks::AddIpcSize(TargetHookSizedCb callback) {
    this->ipc_size.push_back(callback);
    return this;
  }
  TargetHooks* TargetHooks::AddIpcInit(TargetHookIpcMemCb callback) {
    this->ipc_init.push_back(callback);
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

  size_t TargetHooks::ExecAndSumIpcSize() {
    size_t res = 0;
    for (auto cb : this->ipc_size) {
      res += cb();
    }
    return res;
  }

  void TargetHooks::ExecIpcInit(ipc::QueuedSharedMem* ipc_mem) {
    for (auto cb : this->ipc_init) {
      cb(ipc_mem);
    }
  }

  void TargetHooks::ExecPreStart(ipc::QueuedSharedMem* ipc_mem) {
    for (auto cb : this->pre_start) {
      cb(ipc_mem);
    }
  }
  void TargetHooks::ExecPreStartInTarget(ipc::QueuedSharedMem* ipc_mem) {
    for (auto cb : this->pre_start_in_target) {
      cb(ipc_mem);
    }
  }
  void TargetHooks::ExecPostStart(ipc::QueuedSharedMem* ipc_mem, pid_t pid, targets::Target* target) {
    for (auto cb : this->post_start) {
      cb(ipc_mem, pid, target);
    }
  }

  void TargetHooks::ExecPreTest(ipc::QueuedSharedMem* ipc_mem) {
    for (auto cb : this->pre_test) {
      cb(ipc_mem);
    }
  }
  void TargetHooks::ExecPostTest(ipc::QueuedSharedMem* ipc_mem) {
    for (auto cb : this->post_test) {
      cb(ipc_mem);
    }
  }

  void TargetHooks::ExecOnCrash(ipc::QueuedSharedMem* ipc_mem) {
    for (auto cb : this->on_crash) {
      cb(ipc_mem);
    }
  }

  void TargetHooks::ExecPreStop(ipc::QueuedSharedMem* ipc_mem, pid_t pid) {
    for (auto cb : this->pre_stop) {
      cb(ipc_mem, pid);
    }
  }
  void TargetHooks::ExecPostStop(ipc::QueuedSharedMem* ipc_mem, pid_t pid) {
    for (auto cb : this->post_stop) {
      cb(ipc_mem, pid);
    }
  }

}
}
