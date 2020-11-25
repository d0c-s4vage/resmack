#include <cstring>
#include <stdio.h>

#include "resmack/fuzz/feedbacks/noop.hpp"

namespace resmack {
namespace fuzz {

  void NoopCoverage::Start() {
  }

  void NoopCoverage::Stop() {
  }

  FeedbackStats NoopCoverage::GetStats() {
    return { 0, 0 };
  }

}
}
