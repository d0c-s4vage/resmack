#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "resmack/fuzz/ipc/shared_mem_lock.hpp"

namespace resmack {
namespace fuzz {
namespace ipc {

  SharedMemLock::SharedMemLock() {
    this->Init();
  }

  void SharedMemLock::Init() {
    pthread_mutexattr_t attrs;
    pthread_mutexattr_init(&attrs);
    pthread_mutexattr_settype(&attrs, PTHREAD_MUTEX_FAST_NP);
    pthread_mutexattr_setpshared(&attrs, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&this->mutex_, &attrs);
    pthread_mutexattr_destroy(&attrs);
  }

  void SharedMemLock::Lock() {
    int err;
    if ((err = pthread_mutex_lock(&this->mutex_)) != 0) {
      printf("Error locking mutex: %d\n", err);
    }
  }

  void SharedMemLock::Unlock() {
    int err;
    if ((err = pthread_mutex_unlock(&this->mutex_)) != 0) {
      printf("Error unlocking mutex: %d\n", err);
    }
  }

}
}
}
