#ifndef RESMACK_FUZZ_LOCKED_SHARED_MEM_H
#define RESMACK_FUZZ_LOCKED_SHARED_MEM_H

#include <pthread.h>

namespace resmack {
namespace fuzz {
namespace ipc {

  class LockedSharedMem {
   private:
    pthread_mutex_t* mutex_; // uses futex
    void* shared_;
    size_t shared_size_;
    void* root_shared_;
    void* last_shared_;

   public:
    LockedSharedMem(size_t max_size);
    ~LockedSharedMem();

    // Return the pointer to the shared memory, AFTER any bookkeeping
    // structures
    template <typename T>
    T* GetPtr() {
      return (T*)this->root_shared_;
    }

    // A convenience function when identifying offsets within shared_
    template <typename T>
    T* GetNextPtrAfter(size_t size) {
      this->last_shared_ = (void*)((char*)this->last_shared_ + size);
      return (T*)this->last_shared_;
    }
    // Return false if unable to acquire the lock. errno will indicate the
    // error
    bool Acquire();
    // Return false if unable to release the lock. errono will indicate the
    // error
    bool Release();
  };

}
}
}

#endif
