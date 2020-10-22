#ifndef RESMACK_UTILS_HPP
#define RESMACK_UTILS_HPP

#include <stdint.h>

namespace resmack {
namespace utils {

  template <class T>
  static bool OvSub(T num1, T num2, T *dest) {
    T res = num1 - num2;
    if (num1 > 0 && num2 > 0 && res < 0) {
      return false;
    } else if (num1 < 0 && num2 < 0 && res > 0) {
      return false;
    }
    *dest = res;
    return true;
  }

}
}

#endif
