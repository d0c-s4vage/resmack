#include <chrono>
#include <ctime>
#include <ratio>

#include "resmack/fuzz/target.hpp"

namespace resmack {
namespace fuzz {

#define SAMPLED \
  if (this->stats_sample_interval == 0 || (this->sample_ticks % this->stats_sample_interval) != 0) { return; }

  TargetStats::TargetStats(size_t interval): stats_sample_interval(interval) {}

  void TargetStats::Reset() {
    this->exit_code = -1;
    this->timedout = false;
    this->crashed = false;
  }

  // Full reset
  void TargetStats::Clear() {
    this->Reset();
    this->sample_ticks = 0;
    this->mutate_duration = 0;
    this->generate_duration = 0;
    this->setup_duration = 0;
    this->target_duration = 0;
    this->teardown_duration = 0;
  }

  void TargetStats::StartSample(SampleTypes type) {
    SAMPLED

    switch (type) {
      case SampleTypes::SETUP:
        this->setup_start = std::chrono::high_resolution_clock::now();
        break;
      case SampleTypes::TARGET:
        this->target_start = std::chrono::high_resolution_clock::now();
        break;
      case SampleTypes::TEARDOWN:
        this->teardown_start = std::chrono::high_resolution_clock::now();
        break;
      default:
        break;
    }
  }

  void TargetStats::StopSample(SampleTypes type) {
    SAMPLED

    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

    switch (type) {
      case SampleTypes::SETUP:
        this->setup_duration += std::chrono::duration_cast<std::chrono::duration<double>>(now - this->setup_start).count();
        break;
      case SampleTypes::TARGET:
        this->target_duration += std::chrono::duration_cast<std::chrono::duration<double>>(now - this->target_start).count();
        break;
      case SampleTypes::TEARDOWN:
        this->teardown_duration += std::chrono::duration_cast<std::chrono::duration<double>>(now - this->teardown_start).count();
        break;
    }
  }

}
}
