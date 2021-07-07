#ifndef RESMACK_FUZZ_TARGET_NEW_H
#define RESMACK_FUZZ_TARGET_NEW_H

#include <string>
#include <unistd.h>

#include "resmack/fuzz/target_hooks.hpp"
#include "resmack/fuzz/generator.hpp"

namespace resmack {
namespace fuzz {
namespace targets {

  class Target {
   private:
   public:
    virtual ~Target() {}
    virtual pid_t Start() = 0;
    virtual void Stop() = 0;
  };

  enum class TargetType {
    kDirect
  };

  Target* CreateTarget(size_t id, TargetType type, TargetHooks* hooks, size_t max_input_size, Generator* genr);

}
}
}

#endif
