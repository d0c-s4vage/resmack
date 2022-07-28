#ifndef RESMACK_FUZZ_CLI_PARSE_H
#define RESMACK_FUZZ_CLI_PARSE_H

#include "resmack/fuzz/config.hpp"

namespace resmack {
namespace fuzz {
namespace cli {

  void ParseArgs(Config* config, int argc, char** argv);

}
}
}

#endif
