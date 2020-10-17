#include <stdint.h>

namespace resmack {
namespace utils {

  static bool OvSub(int64_t num1, int64_t num2, int64_t *dest) {
    int64_t res = num1 - num2;
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
