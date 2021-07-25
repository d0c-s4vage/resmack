#include <chrono>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>

#include "resmack/fuzz/ipc/queued_shared_mem.hpp"
#include "resmack/types.hpp"
#include "resmack/fuzz/debug.hpp"
#include "resmack/fuzz/cmds/fuzz.hpp"
#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/feedbacks/coverage.hpp"
#include "resmack/fuzz/generator.hpp"
#include "resmack/fuzz/target_new.hpp"
#include "resmack/fuzz/target_hooks.hpp"
#include "resmack/fuzz/tracer.hpp"
#include "resmack/fuzz/states/mmap.hpp"
#include "resmack/fuzz/corpora/mmap.hpp"

namespace resmack {
namespace fuzz {
namespace cmds {

  std::mutex lock;
  bool shouldRun = true;
  size_t iters = 0x1;
  size_t fuzz_n_id_next = 1;

  std::vector<pid_t> forked_pids;

  bool is_parent = true;
  targets::Target* target = nullptr;

  void SignalHandler(int) {
    if (is_parent) {
      for (pid_t pid : forked_pids) {
        kill(pid, SIGTERM);
        waitpid(pid, NULL, 0);
      }
    } else {
      target->Stop();
    }
    _exit(0);
  }

  void* FuzzN(uint32_t id, FuzzConfig*, TargetHooks* hooks, targets::Target* target, Tracer* tracer, corpora::MmapCorpus* corpus, Generator* gen, feedbacks::Coverage* cov) {
    gen->ReinitRand(id);

    size_t count = 0;

    while (shouldRun) {
      count++;
      std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
      target->Start();
      std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

      if (tracer->DidCrash()) {
        CrashInfo* info = tracer->GetCrashInfo();
        // TODO save crash info to disk
        //printf("%lu: Crashed!\nStack:\n%s\n", id, info->minor_stack.c_str());
        //printf("ASAN INFO:\n\n%s\n", info->asan_info);
        hooks->ExecOnCrash(target->GetPrivateMem(), target->GetGlobalMem());
      }
    }

    return NULL;
  }

  void Fuzz(Config* config) {
    signal(SIGINT, SignalHandler);

    TargetHooks hooks;
    states::MmapState state(config->fuzz_config.state_path);
    corpora::MmapCorpus* corpus = state.GetMmapCorpus();

    Generator gen(0, &config->grammar_config, [](Vector<RandSnapshot>*) -> bool {
      // TODO:
      // * Decide if the corpus should be used (randomly)
      // * Choose an item from the corpus
      // * Mutate the item into the RandSnapshot param
      return false;
    });

    bool crashed = false;
    hooks.AddOnCrash([&gen](ipc::QueuedSharedMem*, ipc::QueuedSharedMem*) {
        std::string const* crashing_input = gen.RegenerateLast();
        std::cout << "Crashing input:: " << *crashing_input << std::endl;
    });


    //feedbacks::Feedback* feedback = feedbacks::CreateFeedback(fuzz_config->feedbackType);
    feedbacks::Coverage cov([&gen, corpus](feedbacks::Coverage* this_) {
      printf("New coverage!\n");
      bool descendant_of_last = false;
      //corpus->AddRandSnapshot(gen.GetRand()->GetSnapshots(), this_->GetStats(), descendant_of_last);
    });

    Tracer tracer;

    corpus->InsertHooks(&hooks);
    cov.InsertHooks(&hooks);
    tracer.InsertHooks(&hooks);
    gen.InsertHooks(&hooks);
    state.InsertHooks(&hooks);

    target = targets::CreateTarget(
      0,
      targets::TargetType::kDirect,
      &hooks,
      0x100000,
      &gen
    );

    Vector<pid_t> forked_pids;
    for (size_t i = 0; i < config->fuzz_config.nprocs; i++) {
      pid_t forked_pid = fork();
      if (forked_pid == 0) {
        is_parent = false;
        FuzzN(i, &config->fuzz_config, &hooks, target, &tracer, corpus, &gen, &cov);
        _exit(0);
      } else {
        forked_pids.push_back(forked_pid);
      }
    }

    target->GetPrivateMem()->ListenForUpdates();
    target->GetGlobalMem()->ListenForUpdates();

    while (wait(nullptr) > 0);

    target->GetPrivateMem()->StopListeningForUpdates();
    target->GetGlobalMem()->StopListeningForUpdates();
  }

} // cmds
} // fuzz
} // resmack
