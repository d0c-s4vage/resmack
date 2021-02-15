#ifndef RESMACK_FUZZ_NOOP_FEEDBACK_H
#define RESMACK_FUZZ_NOOP_FEEDBACK_H

#include <stdint.h>
#include <stdlib.h>

#include "resmack/fuzz/feedback.hpp"

namespace resmack {
namespace fuzz {
namespace feedbacks {

  class NoopCoverage : public Feedback {
   public:
    std::string GetSummary();
    FeedbackStats GetStats();
    void InsertHooks(TargetHooks* hooks) {}
  };

}
}
}

#endif
