#ifndef RESMACK_UTILS_H
#define RESMACK_UTILS_H

#include "resmack/rand.hpp"
#include <stdint.h>
#include <string>

namespace resmack {
namespace utils {

  template <class T>
  inline bool OvSub(T num1, T num2, T *dest) {
    T res = num1 - num2;
    if (
        (num1 > 0 && num2 > 0 && res < 0)
        || (num1 < 0 && num2 < 0 && res > 0)
    ) {
      return false;
    }

    *dest = res;
    return true;
  }

  inline void RandBytes(Rand* rand, std::string_view charset, size_t num_chars, std::string* out) {
    if (out->size() + num_chars > out->capacity()) {
      out->reserve(out->size() + num_chars);
    }

    for(uint32_t i = 0; i < num_chars; i++) {
      char c = charset[rand->Next() % charset.size()];
      out->push_back(c);
    }
  }

}
}

#endif
