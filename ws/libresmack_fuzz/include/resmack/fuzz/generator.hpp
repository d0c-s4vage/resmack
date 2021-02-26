#ifndef RESMACK_FUZZ_GENERATOR_H
#define RESMACK_FUZZ_GENERATOR_H

#include <functional>
#include <mutex>
#include <pthread.h>

#include "resmack/rules.hpp"
#include "resmack/rand.hpp"
#include "resmack/fuzz/target_new.hpp"
#include "resmack/fuzz/config.hpp"
#include "resmack/fuzz/ipc/shared_mem_condition.hpp"

namespace resmack {
namespace fuzz {

using ReplayInitCb = std::function<bool(Vector<RandSnapshot> *out)>;
using TargetHookGenericCb = std::function<void()>;

  struct GeneratedInfo {
    ipc::SharedMemCondition ready_cond;
    ipc::SharedMemCondition used_cond;
    std::mutex used_lock;
    std::string output;
    Rand rand;

    GeneratedInfo();
  };

  class Generator {
   private:
    size_t start_rule_idx;
    Rules rules;
    bool to_generate_into;
    bool to_consume_from;
    GeneratedInfo last_generated[2];
    Vector<RandSnapshot> base_replay;
    bool should_run;
    size_t max_depth;
    pthread_t generate_thread;
    ReplayInitCb replay_init_cb;
    size_t id;

    static void* GenerateLoop(void* this_ptr);

   public:
    Generator(size_t id, const GrammarConfig* config, ReplayInitCb cb);
    ~Generator();

    void Run();
    void Stop();

    void NextInputWait();
    const std::string* NextInputGet();
    void NextInputUsed();
  };

}
}

#endif
