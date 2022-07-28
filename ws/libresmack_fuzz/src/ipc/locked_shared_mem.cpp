#include <pthread.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "resmack/fuzz/ipc/locked_shared_mem.hpp"

namespace resmack {
namespace fuzz {
namespace ipc {

  LockedSharedMem::LockedSharedMem(size_t max_size) : lock_(NULL), root_shared_(NULL) {
    this->Init(max_size);
  }

  LockedSharedMem::LockedSharedMem() {}

  LockedSharedMem::~LockedSharedMem() {
    if (this->lock_ != NULL && this->lock_ != MAP_FAILED) {
      munmap(this->lock_, sizeof(SharedMemLock));
    }
    if (this->root_shared_ != NULL && this->root_shared_ != MAP_FAILED) {
      munmap(this->root_shared_, this->shared_size_);
    }
  }

  void LockedSharedMem::Init(size_t max_size) {
    this->shared_size_ = max_size;
    if (this->shared_size_ < (size_t)getpagesize()) {
      this->shared_size_ = getpagesize();
    }

    this->data_size_ = max_size;
    this->lock_ = (SharedMemLock*)mmap(
      NULL, // unnamed
      sizeof(SharedMemLock),
      PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANON,
      -1, // file descriptor
      0 // offset
    );
    if (this->lock_ == MAP_FAILED) {
      perror("Could not create mmap");
      std::exit(1);
    }
    this->lock_->Init();

    this->root_shared_ = mmap(
      NULL, // unnamed
      this->shared_size_,
      PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANON,
      -1, // file descriptor
      0 // offset
    );
    this->last_shared_ = this->root_shared_;
  }

  void LockedSharedMem::Lock() {
    this->lock_->Lock();
  }

  void LockedSharedMem::Unlock() {
    this->lock_->Unlock();
  }

}
}
}
