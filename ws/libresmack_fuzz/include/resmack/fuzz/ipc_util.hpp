#ifndef RESMACK_FUZZ_IPC_UTIL_H
#define RESMACK_FUZZ_IPC_UTIL_H

#include <atomic>
#include <unistd.h>

#include "resmack/debug.hpp"
#include "resmack/fuzz/lock.hpp"

namespace resmack {
  namespace fuzz {
    namespace ipc_util {
      static std::atomic<bool> SHUTTING_DOWN(false);
      static Lock SIGNAL_HANDLER_LOCK;
    }
  }
}

#endif
