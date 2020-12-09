#ifndef RESMACK_FUZZ_IPC_UTIL_H
#define RESMACK_FUZZ_IPC_UTIL_H

#ifdef DEBUG_IPC
#define IPC_DEBUG(MSG) printf(MSG);
#else
#define IPC_DEBUG(MSG)
#endif

#define WITH_LOCK(LOCK, MSG, STATEMENTS) { \
  IPC_DEBUG("Waiting for semaphore "#LOCK" | "#MSG"\n"); \
  if (sem_wait(LOCK) == -1) { \
    if (errno == EINTR) { \
      goto sem_done; \
    } \
    perror(#MSG" (sem_wait)"); \
    std::exit(1); \
  } \
  IPC_DEBUG("-->Done waiting       - "#LOCK" | "#MSG"\n"); \
  while (1) { \
    STATEMENTS \
    break; \
  } \
  IPC_DEBUG("Releasing semaphore - "#LOCK" | "#MSG"\n"); \
  if (sem_post(LOCK) == -1) { \
    perror(#MSG" (sem_post)"); \
    std::exit(1); \
  } \
  IPC_DEBUG("-->Done releasing   - "#LOCK" | "#MSG"\n"); \
  sem_done: ;\
}

#endif
