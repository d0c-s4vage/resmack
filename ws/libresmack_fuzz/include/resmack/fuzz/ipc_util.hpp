#ifndef RESMACK_FUZZ_IPC_UTIL_H
#define RESMACK_FUZZ_IPC_UTIL_H

#define WITH_LOCK(LOCK, MSG, STATEMENTS) { \
  if (sem_wait(LOCK) == -1) { \
    perror("#MSG (sem_wait)"); \
    std::exit(1); \
  } \
  while (1) { \
    STATEMENTS\
    break; \
  } \
  if (sem_post(LOCK) == -1) { \
    perror("#MSG (sem_post)"); \
    std::exit(1); \
  } \
}

#endif
