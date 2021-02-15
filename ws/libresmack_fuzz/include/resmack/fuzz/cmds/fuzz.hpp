#ifndef RESMACK_FUZZ_CMDS_FUZZ_H
#define RESMACK_FUZZ_CMDS_FUZZ_H

#include <mutex>

#include "resmack/rand.hpp"
#include "resmack/types.hpp"
#include "resmack/fuzz/config.hpp"
#include "resmack/fuzz/target_new.hpp"

namespace resmack {
namespace fuzz {
namespace cmds {

  void Fuzz(Config* config);

}
}
}

#endif
