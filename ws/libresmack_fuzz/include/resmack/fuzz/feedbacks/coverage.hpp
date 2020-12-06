#ifndef RESMACK_FUZZ_COVERAGE_H
#define RESMACK_FUZZ_COVERAGE_H

#include <stdint.h>
#include <stdlib.h>

#include "resmack/fuzz/feedback.hpp"

namespace resmack {
namespace fuzz {
  void HandleSanitizerCovTracePcGuard(uint32_t* guard_var);
  void HandleSanitizerCovTracePcGuardInit(uint32_t* start, uint32_t* end);

  class Coverage : public Feedback {
   private:
    size_t hash;

   public:
    void Start();
    void Stop();
    FeedbackStats GetStats();
  };

}
}

#endif
