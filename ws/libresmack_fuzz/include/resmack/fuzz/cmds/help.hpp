#ifndef RESMACK_FUZZ_CMD_HELP_H
#define RESMACK_FUZZ_CMD_HELP_H

#include "resmack/fuzz/config.hpp"

namespace resmack {
namespace fuzz {
namespace cmds {

  void PrintHelp(const char* prog_name, Config* config);

}
}
}

#endif
