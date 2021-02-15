#ifndef RESMACK_FUZZ_FEEDBACK_H
#define RESMACK_FUZZ_FEEDBACK_H

#include <cstddef>
#include <stdint.h>
#include <string>

#include "resmack/fuzz/target_hooks.hpp"

namespace resmack {
namespace fuzz {
namespace feedbacks {

  struct FeedbackStats {
    bool new_coverage;
    size_t key;
    // total number of BBs hit
    size_t num;
  };

  class Feedback {
   public:
    virtual void SyncTargetToShared() {}
    virtual void SyncSharedToTarget() {}
    virtual std::string GetSummary() = 0;
    virtual FeedbackStats GetStats() = 0;
    virtual void InsertHooks(TargetHooks* hooks) = 0;
  };

  enum class FeedbackType {
    kCoverage,
  };

  Feedback* CreateFeedback(FeedbackType type);

}
}
}

#endif
