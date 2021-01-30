#ifndef RESMACK_FUZZ_FEEDBACK_H
#define RESMACK_FUZZ_FEEDBACK_H

#include <cstddef>
#include <stdint.h>
#include <string>

namespace resmack {
namespace fuzz {

  struct FeedbackStats {
    bool new_coverage;
    size_t key;
    // total number of BBs hit
    size_t num;
  };

  class Feedback {
   public:
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void Sync() = 0;
    virtual std::string GetSummary() = 0;
    virtual FeedbackStats GetStats() = 0;
  };

}
}

#endif
