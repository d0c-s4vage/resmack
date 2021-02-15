#include <signal.h>
#include <stdio.h>

#include "resmack/types.hpp"
#include "resmack/fuzz/cmds/fuzz.hpp"
#include "resmack/fuzz/target_new.hpp"
#include "resmack/fuzz/target_hooks.hpp"
#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/feedbacks/coverage.hpp"
#include "resmack/fuzz/tracer.hpp"

namespace resmack {
namespace fuzz {
namespace cmds {

  static bool kRun;

  void SignalHandler(int signal) {
    kRun = false;
  }

  void Fuzz(FuzzConfig* config) {
    //signal(SIGINT, SignalHandler);
    printf("Main fuzz proc, pid: %d\n", getpid());

    TargetHooks hooks;

    Tracer tracer;
    tracer.InsertHooks(&hooks);

    //feedbacks::Feedback* feedback = feedbacks::CreateFeedback(config->feedbackType);
    feedbacks::Coverage cov;
    cov.InsertHooks(&hooks);

    //targets::Target* target = targets::CreateTarget(config->targetType, &hooks);
    targets::Target* target = targets::CreateTarget(
      targets::TargetType::kDirect,
      &hooks,
      0x1000
    );

    target->Start();
    //while (kRun) {
      std::string input = "hello";
      target->Test(&input);
    //}
    target->Stop();

    if (tracer.DidCrash()) {
      CrashInfo* info = tracer.GetCrashInfo();
      printf("Crashed!\nStack:\n%s\n", info->minor_stack.c_str());
    }

    delete target;
  }

}
}
}
