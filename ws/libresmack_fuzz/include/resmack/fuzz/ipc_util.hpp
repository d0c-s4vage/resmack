#ifndef RESMACK_FUZZ_IPC_UTIL_H
#define RESMACK_FUZZ_IPC_UTIL_H

#include <unistd.h>
#include <mutex>

#define IPC_DEBUG_ALWAYS(...) printf(__VA_ARGS__); std::cout << std::flush;

#ifdef DEBUG_IPC
#define IPC_DEBUG(...) IPC_DEBUG_ALWAYS(__VA_ARGS__);
#else
#define IPC_DEBUG(...)
#endif

namespace resmack {
namespace fuzz {
namespace ipc_util {
  static std::mutex SIGNAL_HANDLER_LOCK;
}
}
}

#define WITH_LOCK_DEBUG(LOCK, MSG, STATEMENTS) { \
  resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK.lock();\
  int sem_val;\
  sem_getvalue(LOCK, &sem_val);\
  IPC_DEBUG_ALWAYS("%d: Waiting for semaphore "#LOCK" | "#MSG" (val: %d)\n", getpid(), sem_val); \
  if (sem_wait(LOCK) == -1) { \
    IPC_DEBUG_ALWAYS("%d: ERROR!!!!!!\n", getpid());\
    if (errno == EINTR) { \
      goto sem_done; \
    } \
    perror(#MSG" (sem_wait)"); \
    std::exit(1); \
  } \
  sem_getvalue(LOCK, &sem_val);\
  IPC_DEBUG_ALWAYS("%d: -->Done waiting       "#LOCK" | "#MSG" (val: %d)\n", getpid(), sem_val); \
  while (1) { \
    STATEMENTS \
    break; \
  } \
  sem_getvalue(LOCK, &sem_val);\
  IPC_DEBUG_ALWAYS("%d: Releasing semaphore   "#LOCK" | "#MSG" (val: %d)\n", getpid(), sem_val); \
  if (sem_post(LOCK) == -1) { \
    perror(#MSG" (sem_post)"); \
    std::exit(1); \
  } \
  sem_getvalue(LOCK, &sem_val);\
  IPC_DEBUG_ALWAYS("%d: -->Done releasing     "#LOCK" | "#MSG" (val: %d)\n", getpid(), sem_val); \
  sem_done: ;\
  resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK.unlock();\
}

#define WITH_LOCK(LOCK, MSG, STATEMENTS) { \
  resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK.lock();\
  IPC_DEBUG("%d: Waiting for semaphore "#LOCK" | "#MSG"\n", getpid()); \
  if (sem_wait(LOCK) == -1) { \
    if (errno == EINTR) { \
      goto sem_done; \
    } \
    perror(#MSG" (sem_wait)"); \
    std::exit(1); \
  } \
  IPC_DEBUG("%d: -->Done waiting       "#LOCK" | "#MSG"\n", getpid()); \
  while (1) { \
    STATEMENTS \
    break; \
  } \
  IPC_DEBUG("%d: Releasing semaphore   "#LOCK" | "#MSG"\n", getpid()); \
  if (sem_post(LOCK) == -1) { \
    perror(#MSG" (sem_post)"); \
    std::exit(1); \
  } \
  IPC_DEBUG("%d: -->Done releasing     "#LOCK" | "#MSG"\n", getpid()); \
  sem_done: ;\
  resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK.unlock();\
}

#endif
