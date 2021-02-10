#ifndef RESMACK_FUZZ_SHARED_MEM_LOCK_H
#define RESMACK_FUZZ_SHARED_MEM_LOCK_H

#include <pthread.h>

namespace resmack {
namespace fuzz {
namespace ipc {

  class SharedMemCondition;

  class SharedMemLock {
   friend SharedMemCondition;

   private:
    pthread_mutex_t mutex_;

   public:
    SharedMemLock();
    void Init();
    void Lock();
    void Unlock();
  };

}
}
}

#endif
