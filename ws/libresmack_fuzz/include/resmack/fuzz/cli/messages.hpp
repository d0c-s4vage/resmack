#ifndef RESMACK_FUZZ_CLI_MESSAGES_H
#define RESMACK_FUZZ_CLI_MESSAGES_H

#include <stdio.h>

namespace resmack {
namespace fuzz {
namespace cli {

  void Info(const char* message);
  void Warn(const char* message);
  void Error(const char* message);

}
}
}

#endif
