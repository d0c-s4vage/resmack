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

  std::mutex lock;
  bool shouldRun = true;
  size_t iters = 0x1;
  size_t fuzz_n_id_next = 1;

  std::vector<targets::Target*> targets;

  void SignalHandler(int signum) {
    _DEBUG_PRINT("Handling signal: %d - %s\n", signum, strsignal(signum));
    shouldRun = false;
    for (targets::Target* target : targets) {
      target->Stop();
    }
    _exit(0);
  }

  void* FuzzN(void* config_arg) {
    Config* config = reinterpret_cast<Config*>(config_arg);
    FuzzConfig* fuzz_config = &config->fuzz_config;

    TargetHooks hooks;
    Tracer tracer;
    feedbacks::Coverage cov;

    //feedbacks::Feedback* feedback = feedbacks::CreateFeedback(fuzz_config->feedbackType);
    tracer.InsertHooks(&hooks);
    cov.InsertHooks(&hooks);

    lock.lock();
      size_t id = fuzz_n_id_next++;
    lock.unlock();

    Generator gen(id, &config->grammar_config, [](Vector<RandSnapshot>*) -> bool {
      // no replays for now!
      // TODO hook up to the corpus
      return false;
    });

    targets::Target* target = targets::CreateTarget(
      id,
      targets::TargetType::kDirect,
      &hooks,
      0x100000,
      &gen
    );

    lock.lock();
      targets.push_back(target);
    lock.unlock();

    size_t count = 0;

    while (shouldRun) {
      std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
      target->Start();
      std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
      /*
      printf(
        "%lu: Done, %lu iters in %.03fs = %.03f iters/s\n",
        id,
        iters,
        span.count(),
        (double)iters / span.count()
      );
      */
      tracer.WaitForEvent();

      if (tracer.DidCrash()) {
        CrashInfo* info = tracer.GetCrashInfo();
        printf("%lu: Crashed!\nStack:\n%s\n", id, info->minor_stack.c_str());
      }

      if (++count == iters) {
        break;
      }
    }

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

    while (targets.size() > 0) {
      targets::Target* target = targets.back();
      targets.pop_back();
      delete target;
    }
  }

} // cmds
} // fuzz
} // resmack
