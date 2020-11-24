#include <chrono>
#include <ctime>
#include <ratio>

#include "resmack/fuzz/target.hpp"

namespace resmack {
namespace fuzz {

#define SAMPLED \
  if (this->stats_sample_interval == 0 || (this->sample_ticks % this->stats_sample_interval) != 0) { return; }

  TargetStats::TargetStats(size_t interval):
#define STAT(NAME) duration_##NAME(0),
#include "resmack/fuzz/stats.def"
#undef STAT
    stats_sample_interval(interval)
  {
  }

  void TargetStats::Reset() {
    this->exit_code = -1;
    this->timedout = false;
    this->crashed = false;
  }

  // Full reset
  void TargetStats::Clear() {
    this->Reset();

#define STAT(NAME) this->duration_##NAME = 0;
#include "resmack/fuzz/stats.def"
#undef STAT
  }

  void TargetStats::StartSample(SampleTypes type) {
    SAMPLED

    switch (type) {
#define STAT(NAME) \
      case SampleTypes::NAME: \
        this->start_##NAME = std::chrono::high_resolution_clock::now(); \
        break;

#include "resmack/fuzz/stats.def"

#undef STAT
    }
  }

  void TargetStats::StopSample(SampleTypes type) {
    SAMPLED

    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

    switch (type) {
#define STAT(NAME) \
      case SampleTypes::NAME: \
        this->duration_##NAME = std::chrono::duration_cast<std::chrono::duration<double>>(now - this->start_##NAME).count(); \
        break;

#include "resmack/fuzz/stats.def"

#undef STAT
    }
  }

}
}
