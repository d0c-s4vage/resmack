#ifndef RESMACK_FUZZ_LOCKED_SHARED_MEM_H
#define RESMACK_FUZZ_LOCKED_SHARED_MEM_H

#include <pthread.h>

#include "resmack/fuzz/ipc/shared_mem_lock.hpp"

namespace resmack {
namespace fuzz {
namespace ipc {

  class LockedSharedMem {
   private:
    SharedMemLock* lock_;
    // total size of the shared memory
    size_t shared_size_;
    // size of the user-data within the shared memory
    size_t data_size_;
    void* root_shared_;
    void* last_shared_;

   public:
    LockedSharedMem(size_t max_size);
    // Init() must be explicitly called if a sized constructor is not used
    LockedSharedMem();
    ~LockedSharedMem();

    void Init(size_t max_size);
    size_t DataSize() { return this->data_size_; }

    // Return the pointer to the shared memory, AFTER any bookkeeping
    // structures
    template <typename T>
    T* GetPtr() {
      return (T*)this->root_shared_;
    }

    // A convenience function when identifying offsets within shared_
    template <typename T>
    T* GetNextPtrFor(size_t size) {
      T* res = (T*)this->last_shared_;
      this->last_shared_ = (void*)((char*)this->last_shared_ + size);
      return res;
    }

    // A convenience function when identifying offsets within shared_
    template <typename T>
    T* GetNextPtrFor() {
      return this->GetNextPtrFor<T>(sizeof(T));
    }

    void Lock();
    void Unlock();
  };

}
}
}

#endif
