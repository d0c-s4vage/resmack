#include <chrono>
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
    kRun = true;
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

    int count = 0;
    target->Start();
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    while (kRun) {
      std::string input = "hello";
      int res = target->Test(&input);
      count++;
      if (count == 0x30000) {
        break;
      }
    }
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    printf("%0.03f iters/s\n", (double)count / span.count());
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
