#include "resmack/fuzz/target_hooks.hpp"

namespace resmack {
namespace fuzz {

  TargetHooks::TargetHooks() {}

  TargetHooks* TargetHooks::AddPreStart(TargetHookCb callback) {
    this->pre_start.push_back(callback);
    return this;
  }
  TargetHooks* TargetHooks::AddPostStart(TargetHookCb callback) {
    this->post_start.push_back(callback);
    return this;
  }

  TargetHooks* TargetHooks::AddPreTest(TargetHookCb callback) {
    this->pre_test.push_back(callback);
    return this;
  }
  TargetHooks* TargetHooks::AddPostTest(TargetHookCb callback) {
    this->post_test.push_back(callback);
    return this;
  }

  TargetHooks* TargetHooks::AddPreStop(TargetHookCb callback) {
    this->pre_stop.push_back(callback);
    return this;
  }
  TargetHooks* TargetHooks::AddPostStop(TargetHookCb callback) {
    this->post_stop.push_back(callback);
    return this;
  }

  void TargetHooks::ExecPreStart() {
    for (auto cb : this->pre_start) {
      cb();
    }
  }
  void TargetHooks::ExecPostStart() {
    for (auto cb : this->post_start) {
      cb();
    }
  }

  void TargetHooks::ExecPreTest() {
    for (auto cb : this->pre_test) {
      cb();
    }
  }
  void TargetHooks::ExecPostTest() {
    for (auto cb : this->post_test) {
      cb();
    }
  }

  void TargetHooks::ExecPreStop() {
    for (auto cb : this->pre_stop) {
      cb();
    }
  }
  void TargetHooks::ExecPostStop() {
    for (auto cb : this->post_stop) {
      cb();
    }
  }

}
}
