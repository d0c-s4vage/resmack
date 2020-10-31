#ifndef RESMACK_BUILD_CTX
#define RESMACK_BUILD_CTX

#include <string>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <cstddef>
#include <string>

#include "resmack/rand.hpp"

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

    bool DoShortest();
    size_t IncDepth();
    size_t DecDepth();
    void PrintDebugIo();
    void Message(std::string msg);
  };

}

#endif
