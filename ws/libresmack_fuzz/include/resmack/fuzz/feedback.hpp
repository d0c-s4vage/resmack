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
    virtual void FPOStart() noexcept = 0;
    virtual void FPOStop() noexcept = 0;
    virtual bool Sync() noexcept = 0;
    virtual std::string GetSummary() const noexcept = 0;
    virtual FeedbackStats GetStats() const noexcept = 0;
  };

}
}

#endif
