#ifndef RESMACK_FUZZ_SHARED_MEM_CONDITION_H
#define RESMACK_FUZZ_SHARED_MEM_CONDITION_H

#include <pthread.h>

#include "resmack/fuzz/ipc/shared_mem_lock.hpp"

namespace resmack {
namespace fuzz {
namespace ipc {

  class SharedMemCondition {
   private:
    SharedMemLock lock_;
    pthread_cond_t condition_;
    bool val_;

   public:
    SharedMemCondition(bool shared);
    void Init(bool shared);

    void Wait();
    void WaitAndHold();
    void WaitRaw_Danger();

    void Signal();
    void SignalRaw_Danger();

    void Reset();

    void Lock();
    void Unlock();
  };

}
}
}

#endif
