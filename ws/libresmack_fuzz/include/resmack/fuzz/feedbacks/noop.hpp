#ifndef RESMACK_FUZZ_NOOP_FEEDBACK_H
#define RESMACK_FUZZ_NOOP_FEEDBACK_H

#include <stdint.h>
#include <stdlib.h>

#include "resmack/fuzz/feedback.hpp"

namespace resmack {
namespace fuzz {

  class NoopCoverage : public Feedback {
   public:
    void Start();
    void Stop();
    FeedbackStats GetStats() const noexcept;
  };

}
}

#endif
