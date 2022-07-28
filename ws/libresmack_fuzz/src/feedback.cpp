#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/feedbacks/coverage.hpp"

namespace resmack {
namespace fuzz {
namespace feedbacks {

  Feedback* CreateFeedback(FeedbackType type) {
    return nullptr;
    /*
    switch(type) {
      case FeedbackType::kCoverage:
        return static_cast<Feedback*>(new Coverage());
        break;
    }
    */
  }

}
}
}
