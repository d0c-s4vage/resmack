#ifndef RESMACK_FUZZ_COVERAGE_H
#define RESMACK_FUZZ_COVERAGE_H

#include <stdint.h>
#include <stdlib.h>

#include "resmack/fuzz/feedback.hpp"

extern "C" void __sanitizer_cov_trace_pc_guard(uint32_t* guard_var);
extern "C" void __sanitizer_cov_trace_pc_guard_init(uint32_t* start, uint32_t* end);

namespace resmack {
namespace fuzz {

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
