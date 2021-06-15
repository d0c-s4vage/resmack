#include <chrono>
#include <signal.h>
#include <stdio.h>

#include "resmack/types.hpp"
#include "resmack/fuzz/debug.hpp"
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
  size_t iters = 0x3000;
  size_t fuzz_n_id_next = 1;

  std::vector<targets::Target*> targets;

  void SignalHandler(int signum) {
    _DEBUG_PRINT("Handling signal: %d - %s\n", signum, strsignal(signum));
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
    feedbacks::Coverage cov;

    //feedbacks::Feedback* feedback = feedbacks::CreateFeedback(fuzz_config->feedbackType);
    tracer.InsertHooks(&hooks);
    cov.InsertHooks(&hooks);

    lock.lock();
      size_t id = fuzz_n_id_next++;
    lock.unlock();

    //targets::Target* target = targets::CreateTarget(fuzz_config->targetType, &hooks);
    targets::Target* target = targets::CreateTarget(
      id,
      targets::TargetType::kDirect,
      &hooks,
      0x100000
    );

    lock.lock();
      targets.push_back(target);
    lock.unlock();

    Generator gen(id, &config->grammar_config, [](Vector<RandSnapshot>*) -> bool {
      // no replays for now!
      // TODO hook up to the corpus
      return false;
    });
    gen.Run();

    size_t count = 0;
    target->Start();
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    const std::string* next_input;
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
    _DEBUG_PRINT(
      "%lu: Done, %lu iters in %.03fs = %.03f iters/s\n",
      id,
      iters,
      span.count(),
      (double)iters / span.count()
    );
    target->Stop();

    if (tracer.DidCrash()) {
      CrashInfo* info = tracer.GetCrashInfo();
      _DEBUG_PRINT("%lu: Crashed!\nStack:\n%s\n", id, info->minor_stack.c_str());
    }

    gen.Stop();

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
    _DEBUG_PRINT(
      "Total iters: %lu in %.03fs = %.03fs iters/s\n",
      total_iters,
      span.count(),
      (double)total_iters / span.count()
    );

    while (targets.size() > 0) {
      targets::Target* target = targets.back();
      targets.pop_back();
      delete target;
    }
  }

} // cmds
} // fuzz
} // resmack
