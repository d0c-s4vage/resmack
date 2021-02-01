#ifndef RESMACK_FUZZ_IPC_UTIL_H
#define RESMACK_FUZZ_IPC_UTIL_H

#include <unistd.h>
#include <semaphore.h>

namespace resmack {
  namespace fuzz {
    namespace ipc_util {
      static bool SIGNAL_HANDLER_LOCK_INITED = false;
      static sem_t* SIGNAL_HANDLER_LOCK;
    }
  }
}

#define DEBUG_IPC
#define DEBUG_MESSAGES

#define _DEBUG_PRINT(...) printf(__VA_ARGS__); std::cout << std::flush;
#define IPC_DEBUG_ALWAYS(...) DEBUG_PRINT(__VA_ARGS__)

#define MAKE_SIGNAL_SAFE(MSG) \
  if (resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK_INITED && \
      sem_wait(resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK) == -1) {\
    perror(#MSG" (sem_wait)"); \
    std::exit(1); \
  }

#define _WITH_LOCK_DEBUG(LOCK, MSG, STATEMENTS) { \
  int sem_val;\
  IPC_DEBUG_ALWAYS("%d: siglock inited?      "#LOCK" | "#MSG" inited: %d\n", getpid(), resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK_INITED);\
  if (resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK_INITED) {\
    sem_getvalue(resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK, &sem_val);\
    IPC_DEBUG_ALWAYS("%d: Waiting for siglock   "#LOCK" | "#MSG" (val: %d), inited: %d\n", getpid(), sem_val, resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK_INITED);\
    if (sem_wait(resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK) == -1) {\
      IPC_DEBUG_ALWAYS("%d: ERROR!!!!!!\n", getpid());\
      if (errno == EINTR) { \
        goto sem_done; \
      } \
      perror(#MSG" (sem_wait)"); \
      std::exit(1); \
    }\
  }\
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
  if (resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK_INITED) {\
    sem_getvalue(resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK, &sem_val);\
    IPC_DEBUG_ALWAYS("%d: Releasing semap siglk "#LOCK" | "#MSG" (val: %d)\n", getpid(), sem_val); \
    if (sem_post(resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK) == -1) { \
      perror(#MSG" (sem_post)"); \
      std::exit(1); \
    }\
  } \
}

#define _WITH_LOCK(LOCK, MSG, STATEMENTS) { \
  if (resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK_INITED && \
      sem_wait(resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK) == -1) {\
    if (errno == EINTR) { \
      goto sem_done; \
    } \
    perror(#MSG" (sem_wait)"); \
    std::exit(1); \
  }\
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
  if (resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK_INITED && \
      sem_post(resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK) == -1) { \
    perror(#MSG" (sem_post)"); \
    std::exit(1); \
  } \
}

#ifdef DEBUG_IPC
#define WITH_LOCK(...) _WITH_LOCK_DEBUG(__VA_ARGS__)
#else
#define WITH_LOCK(...) _WITH_LOCK(__VA_ARGS__)
#define DEBUG_PRINT(...)
#endif

#ifdef DEBUG_MESSAGES
#define DEBUG_PRINT(...) _DEBUG_PRINT(__VA_ARGS__)
#endif

#endif
