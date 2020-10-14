#ifndef RESMACK_BUILD_CTX
#define RESMACK_BUILD_CTX

#include <string>

#include "rand.hpp"

namespace resmack {

class Rules;

struct BuildContext {
  Rules *rules;
  std::string *pre_output;
  std::string *output;
  Rand *rand;
};

}

#endif
