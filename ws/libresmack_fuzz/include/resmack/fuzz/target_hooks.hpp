#ifndef RESMACK_FUZZ_TARGET_HOOKS_H
#define RESMACK_FUZZ_TARGET_HOOKS_H

#include <functional>

#include "resmack/types.hpp"

namespace resmack {
namespace fuzz {

  using TargetHookCb = std::function<void()>;

  class TargetHooks {
   private:
    Vector<TargetHookCb> pre_start;
    Vector<TargetHookCb> post_start;

    Vector<TargetHookCb> pre_test;
    Vector<TargetHookCb> post_test;

    Vector<TargetHookCb> pre_stop;
    Vector<TargetHookCb> post_stop;

   public:
    TargetHooks();

    TargetHooks* AddPreStart(TargetHookCb call_back);
    TargetHooks* AddPostStart(TargetHookCb call_back);

    TargetHooks* AddPreTest(TargetHookCb call_back);
    TargetHooks* AddPostTest(TargetHookCb call_back);

    TargetHooks* AddPreStop(TargetHookCb call_back);
    TargetHooks* AddPostStop(TargetHookCb call_back);

    void ExecPreStart();
    void ExecPostStart();

    void ExecPreTest();
    void ExecPostTest();

    void ExecPreStop();
    void ExecPostStop();
  };

}
}

#endif
