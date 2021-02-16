#include <chrono>
#include <signal.h>
#include <stdio.h>

#include "resmack/types.hpp"
#include "resmack/fuzz/cmds/fuzz.hpp"
#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/feedbacks/coverage.hpp"
#include "resmack/fuzz/generator.hpp"
#include "resmack/fuzz/target_new.hpp"
#include "resmack/fuzz/target_hooks.hpp"
#include "resmack/fuzz/tracer.hpp"

namespace resmack {
namespace fuzz {
namespace cmds {

  static bool kRun;
  std::mutex lock;
  size_t iters = 0x30000;

  std::vector<targets::Target*> targets;

  void SignalHandler(int signum) {
    for (targets::Target* target : targets) {
      target->Stop();
    }
    _exit(0);
  }

  void* FuzzN(void* config_arg) {
    Config* config = reinterpret_cast<Config*>(config_arg);
    FuzzConfig* fuzz_config = &config->fuzz_config;
    kRun = true;

    TargetHooks hooks;
    Tracer tracer;
    tracer.InsertHooks(&hooks);

    //feedbacks::Feedback* feedback = feedbacks::CreateFeedback(fuzz_config->feedbackType);
    feedbacks::Coverage cov;
    cov.InsertHooks(&hooks);

    //targets::Target* target = targets::CreateTarget(fuzz_config->targetType, &hooks);
    targets::Target* target = targets::CreateTarget(
      targets::TargetType::kDirect,
      &hooks,
      0x1000
    );

    lock.lock();
      targets.push_back(target);
    lock.unlock();

    Generator gen(&config->grammar_config, [](Vector<RandSnapshot>*) -> bool {
      // no replays for now!
      // TODO hook up to the corpus
      return false;
    });
    gen.Run();

    size_t count = 0;
    target->Start();
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    while (kRun) {
      gen.NextInputWait();
      const std::string* next_input = gen.NextInputGet();
        target->Test(next_input);
      gen.NextInputUsed();

      if (++count == iters) {
        break;
      }
    }
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    printf("Done, %lu iters in %.03fs = %.03f iters/s\n", iters, span.count(), (double)iters / span.count());
    target->Stop();

    if (tracer.DidCrash()) {
      CrashInfo* info = tracer.GetCrashInfo();
      printf("Crashed!\nStack:\n%s\n", info->minor_stack.c_str());
    }

    gen.Stop();

    delete target;

    return NULL;
  }

  void Fuzz(Config* config) {
    signal(SIGINT, SignalHandler);

    std::vector<pthread_t> threads;
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < config->fuzz_config.nprocs; i++) {
      threads.emplace_back();
      pthread_t* curr_thread = &threads[threads.size()-1];
      pthread_create(
        curr_thread,
        NULL,
        &FuzzN,
        (void*)config
      );
    }

    for (pthread_t& thread : threads) {
      pthread_join(thread, NULL);
    }
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

    size_t total_iters = iters * config->fuzz_config.nprocs;
    printf(
      "Total iters: %lu in %.03fs = %.03fs iters/s\n",
      total_iters,
      span.count(),
      (double)total_iters / span.count()
    );
  }

}
}
}
