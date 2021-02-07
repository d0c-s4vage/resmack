#ifndef RESMACK_FUZZ_IPC_UTIL_H
#define RESMACK_FUZZ_IPC_UTIL_H

#include <unistd.h>
#include <semaphore.h>

#include "resmack/fuzz/lock.hpp"

namespace resmack {
  namespace fuzz {
    namespace ipc_util {
      static Lock SIGNAL_HANDLER_LOCK;
    }
  }
}

#define DEBUG_IPC
#define DEBUG_MESSAGES

#define _DEBUG_PRINT(...) printf(__VA_ARGS__); std::cout << std::flush;
#define IPC_DEBUG_ALWAYS(...) DEBUG_PRINT(__VA_ARGS__)

#define _WITH_LOCK_DEBUG(LOCK, MSG, STATEMENTS) { \
  int sem_val;\
  resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK.Acquire();\
  \
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
  \
  sem_done: ;\
  resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK.Release();\
}

#define _WITH_LOCK(LOCK, MSG, STATEMENTS) { \
  resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK.Acquire();\
  if (sem_wait(LOCK) == -1) { \
    if (errno == EINTR) { \
      goto sem_done; \
    } \
    perror(#MSG" (sem_wait)"); \
    std::exit(1); \
  } \
  while (1) { \
    STATEMENTS \
    break; \
  } \
  if (sem_post(LOCK) == -1) { \
    perror(#MSG" (sem_post)"); \
    std::exit(1); \
  } \
  sem_done: ;\
  resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK.Release();\
}

#ifdef DEBUG_IPC
#define WITH_LOCK(...) _WITH_LOCK_DEBUG(__VA_ARGS__)
#else
#define WITH_LOCK(...) _WITH_LOCK(__VA_ARGS__)
#endif

#endif
