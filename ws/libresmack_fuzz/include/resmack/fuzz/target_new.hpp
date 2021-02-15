#ifndef RESMACK_FUZZ_TARGET_NEW_H
#define RESMACK_FUZZ_TARGET_NEW_H

#include <string>
#include <unistd.h>

#include "resmack/fuzz/target_hooks.hpp"

namespace resmack {
namespace fuzz {
namespace targets {

  class Target {
   private:
   public:
    virtual ~Target() {}
    virtual pid_t Start() = 0;
    virtual int Test(const std::string* input) = 0;
    virtual void Stop() = 0;
    virtual void ForceFinishTest() = 0;
  };

  enum class TargetType {
    kDirect
  };

  Target* CreateTarget(TargetType type, TargetHooks* hooks, size_t max_input_size);

}
}
}

#endif
