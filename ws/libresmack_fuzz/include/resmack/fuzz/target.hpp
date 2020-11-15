#ifndef RESMACK_FUZZ_TARGET
#define RESMACK_FUZZ_TARGET

#include <string>

#include "resmack/fuzz/feedback.hpp"

namespace resmack {
namespace fuzz {
  struct TargetSettings {
    // timeout
  };

  enum SampleTypes {
    SETUP,
    TARGET,
    TEARDOWN,
  };

#define RECORD_STAT(STAT, SAMPLE_TYPE, BLOCK) { \
  STAT->StartSample(SAMPLE_TYPE); \
  { BLOCK } \
  STAT->StopSample(SAMPLE_TYPE); \
}

  struct TargetStats {
    int exit_code;
    // timeout
    bool timedout;
    bool crashed;
    // crash_info*
    // 
    size_t stats_sample_interval;
    size_t sample_ticks;
    //
    size_t mutate_duration;
    size_t generate_duration;
    size_t setup_duration;
    size_t target_duration;
    size_t teardown_duration;

    TargetStats();
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
  };

}
}

#endif
