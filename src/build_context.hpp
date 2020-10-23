#ifndef RESMACK_BUILD_CTX
#define RESMACK_BUILD_CTX

#include <string>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <cstddef>
#include <string>

#include "rand.hpp"

namespace resmack {

class Rules;

  class BuildContext {
   public:
    Rules *rules;
    std::string *pre_output;
    std::string *output;
    Rand *rand;
    size_t ref_depth;
    size_t max_depth;

    bool DoShortest() {
      return this->ref_depth >= this->max_depth;
    }

    size_t IncDepth() {
      if (this->ref_depth == std::numeric_limits<size_t>::max()) {
        throw std::overflow_error("Attempted to incremenet ref depth past size_t max");
      }
      return this->ref_depth++;
    }
    size_t DecDepth() {
      if (this->ref_depth == 0) {
        throw std::overflow_error("Attempted to decrement ref depth past 0");
      }
      return this->ref_depth--;
    }

    void Message(std::string msg) {
      std::string indent;
      for (size_t i = 0; i < this->ref_depth; i++) {
        indent += "  ";
      }
      indent += std::to_string(this->ref_depth) + "/" + std::to_string(this->max_depth);
      indent += "- ";

      size_t last_idx = 0;
      size_t newline_idx = msg.find("\n", last_idx);
      while (newline_idx != std::string::npos) {
        std::cout << indent << msg.substr(last_idx, newline_idx - last_idx) << std::endl;
      }
      std::cout << indent << msg.substr(last_idx, msg.size() - last_idx) << std::endl;
    }
  };

}

#endif
