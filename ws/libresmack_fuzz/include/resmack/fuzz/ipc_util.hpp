#ifndef RESMACK_FUZZ_IPC_UTIL_H
#define RESMACK_FUZZ_IPC_UTIL_H

#include <atomic>
#include <unistd.h>
#include <mutex>

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

#define _WITH_LOCK(LOCK, MSG, STATEMENTS) { \
  { \
    std::scoped_lock __l(LOCK); \
    while (1) { \
      STATEMENTS \
      break; \
    } \
  } \
}

#define WITH_LOCK(...) _WITH_LOCK(__VA_ARGS__)

#endif
