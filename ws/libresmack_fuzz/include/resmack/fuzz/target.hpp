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
    CORPUS,
    MUTATE,
    GENERATE,
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

    std::chrono::high_resolution_clock::time_point mutate_start;
    double mutate_duration;

    std::chrono::high_resolution_clock::time_point generate_start;
    double generate_duration;

    std::chrono::high_resolution_clock::time_point setup_start;
    double setup_duration;

    std::chrono::high_resolution_clock::time_point target_start;
    double target_duration;

    std::chrono::high_resolution_clock::time_point teardown_start;
    double teardown_duration;


    TargetStats(size_t intervalv);
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
