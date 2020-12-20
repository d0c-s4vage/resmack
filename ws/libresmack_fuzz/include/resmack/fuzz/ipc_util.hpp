#ifndef RESMACK_FUZZ_IPC_UTIL_H
#define RESMACK_FUZZ_IPC_UTIL_H

#ifdef DEBUG_IPC
#include <unistd.h>

#define IPC_DEBUG(...) printf(__VA_ARGS__);
#else
#define IPC_DEBUG(...)
#endif

#define WITH_LOCK(LOCK, MSG, STATEMENTS) { \
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
}

#endif
