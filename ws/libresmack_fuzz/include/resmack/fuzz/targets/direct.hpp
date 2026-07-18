#ifndef RESMACK_FUZZ_TARGET_DIRECT
#define RESMACK_FUZZ_TARGET_DIRECT

#include "resmack/fuzz/target.hpp"

namespace resmack {
namespace fuzz {

  class DirectTarget: Target {
   public:
    DirectTarget();

    void Launch(Feedback* feedback,
                std::string* output,
                TargetSettings* settings,
                TargetStats* stats);
    void Reset();
  };

}
}

#endif
