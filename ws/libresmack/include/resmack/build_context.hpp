#ifndef RESMACK_BUILD_CTX
#define RESMACK_BUILD_CTX

#include <string>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <cstddef>
#include <string>

#include "resmack/rand.hpp"
#include "resmack/types.hpp"

namespace resmack {

  class Rules;

  class BuildContext {
   public:
    Rules *rules;
    std::string *pre_output;
    std::string *output;
    std::string *post_output;
    Rand *rand;
    size_t ref_depth;
    size_t max_depth;

    Vector<RandSnapshot>* replay;
    size_t replay_idx;
    bool did_replay;

    BuildContext(std::string* output, Rand* rand, size_t max_depth);

    // reset replay and other state fields
    void Reset();
    void MaybeDoRandReplay(uint32_t tmp_state[]);
    void MaybeUndoRandReplay(uint32_t tmp_state[]);

    bool DoShortest();
    size_t IncDepth();
    size_t DecDepth();
    void PrintDebugIo();
    void Message(std::string msg);
  };

}

#endif
