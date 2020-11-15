#include <ctime>

#include "resmack/fuzz/target.hpp"

namespace resmack {
namespace fuzz {

#define SAMPLED \
  if (this->sample_ticks % this->stats_sample_interval != 0) { return; }

  TargetStats::TargetStats() {}

  void TargetStats::Reset() {
    this->exit_code = -1;
    this->timedout = false;
    this->crashed = false;
  }

  // Full reset
  void TargetStats::Clear() {
    this->Reset();
    this->sample_ticks = 0u;
    this->mutate_duration = 0u;
    this->generate_duration = 0u;
    this->setup_duration = 0u;
    this->target_duration = 0u;
    this->teardown_duration = 0u;
  }

  void TargetStats::StartSample(SampleTypes type) {
    SAMPLED

    switch (type) {
      case SampleTypes::SETUP:
        this->setup_duration = clock();
        break;
      case SampleTypes::TARGET:
        this->target_duration = clock();
        break;
      case SampleTypes::TEARDOWN:
        this->teardown_duration = clock();
        break;
      default:
        break;
    }
  }

  void TargetStats::StopSample(SampleTypes type) {
    SAMPLED

    switch (type) {
      case SampleTypes::SETUP:
        this->setup_duration = clock() - this->setup_duration;
        break;
      case SampleTypes::TARGET:
        this->target_duration = clock() - this->target_duration;
        break;
      case SampleTypes::TEARDOWN:
        this->teardown_duration = clock() - this->teardown_duration;
        break;
    }
  }

}
}
