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
  class Item;

  class BuildContext {
   public:
    Rules* rules;
    std::string* pre_output;
    std::string* output;
    Rand* rand;
    uint32_t ref_depth;
    uint32_t max_depth;
    Vector<Item*> post_items;

    const Vector<RandSnapshot>* replay;
    size_t replay_idx;

    BuildContext(std::string* output, Rand* rand, uint32_t max_depth);
    ~BuildContext();

    // reset replay and other state fields
    void SetReplay(const Vector<RandSnapshot>* replay);
    void MaybeDoRandReplay(uint32_t tmp_state[], uint32_t* tmp_max_depth, bool* tmp_did_replay);
    void MaybeUndoRandReplay(uint32_t tmp_state[], uint32_t tmp_max_depth, bool tmp_did_replay);

    bool DoShortest();
    size_t IncDepth();
    size_t DecDepth();
    void PrintDebugIo();
    void Message(std::string msg);
    void FlushPrePost();
    void AddPostItem(Item* item);
  };

}

#endif
