#ifndef RESMACK_FUZZ_IPC_QUEUED_UPDATE_TYPES_H
#define RESMACK_FUZZ_IPC_QUEUED_UPDATE_TYPES_H

#include <inttypes.h>

namespace resmack {
namespace fuzz {
namespace ipc {

  enum QueuedUpdateTypes: uint16_t {
    STATE,
    CORPUS,
    COVERAGE
  };

}
}
}

#endif
