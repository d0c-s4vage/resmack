#include <cstring>
#include <stdio.h>

#include "resmack/fuzz/feedbacks/noop.hpp"

namespace resmack {
namespace fuzz {
namespace feedbacks {

  std::string NoopCoverage::GetSummary() {
    return "NOOP";
  }

  FeedbackStats NoopCoverage::GetStats() {
    return { 0, 0, 0 };
  }

}
}
}
