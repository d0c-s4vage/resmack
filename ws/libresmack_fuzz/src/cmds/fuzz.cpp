#include <stdio.h>

#include "resmack/fuzz/cmds/fuzz.hpp"

namespace resmack {
namespace fuzz {
namespace cmds {

  void Fuzz(FuzzConfig* config) {
    printf("Fuzzing! State path: %s\n", config->state_path);

    /*
    Rules rules;
    Rand rand;

    TargetHooks hooks;
    std::unique_ptr<Target> target = targets::CreateTarget(config->targetType);
    */
  }

}
}
}
