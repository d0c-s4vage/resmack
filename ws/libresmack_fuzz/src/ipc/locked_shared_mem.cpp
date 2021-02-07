#include <pthread.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>

#include "resmack/fuzz/ipc/locked_shared_mem.hpp"

namespace resmack {
namespace fuzz {
namespace ipc {

  LockedSharedMem::LockedSharedMem(size_t max_size) : shared_(NULL) {
    this->shared_size_ = max_size + sizeof(pthread_t);

    this->shared_ = mmap(
      NULL, // unnamed
      this->shared_size_,
      PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANONYMOUS,
      -1, // file descriptor
      0 // offset
    );

    if (this->shared_ == MAP_FAILED) {
      perror("Could not create mmap");
      std::exit(1);
    }

    this->root_shared_ = (void*)((char*)this->shared_ + sizeof(pthread_mutex_t));
    this->last_shared_ = this->root_shared_;

    this->mutex_ = (pthread_mutex_t*)this->shared_;
    pthread_mutexattr_t attrs;
    pthread_mutexattr_init(&attrs);
    pthread_mutexattr_setpshared(&attrs, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(this->mutex_, &attrs); // default initialization
  }

  LockedSharedMem::~LockedSharedMem() {
    if (this->shared_ != NULL && this->shared_ != MAP_FAILED) {
      munmap(this->shared_, this->shared_size_);
    }
  }

  bool LockedSharedMem::Acquire() {
    int err;
    if ((err = pthread_mutex_lock(this->mutex_)) != 0) {
      printf("Error locking mutex: %d\n", err);
      std::exit(1);
    }
    return true;
  }

  bool LockedSharedMem::Release() {
    int err;
    if ((err = pthread_mutex_unlock(this->mutex_)) != 0) {
      printf("Error unlocking mutex: %d\n", err);
      std::exit(1);
    }
    return true;
  }

}
}
}
