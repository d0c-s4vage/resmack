#ifndef RESMACK_FUZZ_GENERATOR_H
#define RESMACK_FUZZ_GENERATOR_H

#include <functional>
#include <mutex>
#include <pthread.h>

#include "resmack/rules.hpp"
#include "resmack/rand.hpp"
#include "resmack/fuzz/config.hpp"
#include "resmack/fuzz/ipc/shared_mem_condition.hpp"

namespace resmack {
namespace fuzz {

using ReplayInitCb = std::function<bool(Vector<RandSnapshot> *out)>;
using TargetHookGenericCb = std::function<void()>;

  class Generator {
   private:
    size_t start_rule_idx;
    Rules rules;
    Vector<RandSnapshot> base_replay;
    size_t max_depth;
    pthread_t generate_thread;
    ReplayInitCb replay_init_cb;
    size_t id;

    std::string output;
    Rand rand;

   public:
    Generator(size_t id, const GrammarConfig* config, ReplayInitCb cb);
    ~Generator();
    std::string const* Generate();
  };

}
}

#endif
