#include "resmack/fuzz/cli/messages.hpp"

#include <stdio.h>

namespace resmack {
namespace fuzz {
namespace cli {

  void Info(const char* message) {
    printf("%s\n", message);
  }

  void Warn(const char* message) {
    printf("%s\n", message);
  }

  void Error(const char* message) {
    printf("%s\n", message);
  }

}
}
}
