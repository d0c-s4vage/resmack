#ifndef RESMACK_FUZZ_TARGET
#define RESMACK_FUZZ_TARGET

#include <chrono>
#include <cstddef>
#include <string>

#include "resmack/fuzz/feedback.hpp"

namespace resmack {
namespace fuzz {
  struct TargetSettings {
    // timeout
  };

  enum SampleTypes {
#define STAT(NAME) NAME,
#include "resmack/fuzz/stats.def"
#undef STAT
  };

#define RECORD_STAT(STAT, SAMPLE_TYPE, BLOCK) { \
  (STAT)->StartSample(SAMPLE_TYPE); \
  { BLOCK } \
  (STAT)->StopSample(SAMPLE_TYPE); \
}

  struct TargetStats {
    int exit_code;
    // timeout
    bool timedout;
    bool crashed;
    bool valid;

#define STAT(NAME) \
  std::chrono::high_resolution_clock::time_point start_##NAME; \
  double duration_##NAME;
#include "resmack/fuzz/stats.def"
#undef STAT
    size_t stats_sample_interval;
    size_t sample_ticks;

    TargetStats(size_t interval);
    void Tick() { this->sample_ticks++; }
    void Reset();
    void Clear();
    void StartSample(SampleTypes type);
    void StopSample(SampleTypes type);
  };

  class Target {
   public:
    virtual void Launch(Feedback* feedback,
                        std::string* output,
                        TargetSettings* settings,
                        TargetStats* stats) = 0;
    virtual void Reset() = 0;
  };

}
}

#endif
