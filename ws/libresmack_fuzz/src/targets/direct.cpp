#include <string>
#include <iostream>

#include "resmack/fuzz/targets/direct.hpp"
#include "resmack/fuzz/target.hpp"
#include "resmack/fuzz/external.hpp"

/*
extern int ResmackTestOneInput(const unsigned char *data, size_t size);
extern int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size);
*/

namespace resmack {
namespace fuzz {

  static ExternalFunctions EF;

  DirectTarget::DirectTarget() {
  }

  void DirectTarget::Launch(
    Feedback* feedback,
    std::string* output,
    TargetSettings* settings,
    TargetStats* stats
  ) {
    UNUSED(settings);

    stats->Reset();

    RECORD_STAT(stats, SampleTypes::FEEDBACK, {
      feedback->Start();
    });

    RECORD_STAT(stats, SampleTypes::TARGET, {
      size_t res = EF.LLVMFuzzerTestOneInput((const uint8_t*)output->c_str(), output->size());
      stats->crashed = res == 1;
    });

    RECORD_STAT(stats, SampleTypes::FEEDBACK, {
      feedback->Stop();
    });
  }

  void DirectTarget::Reset() {}

}
}
