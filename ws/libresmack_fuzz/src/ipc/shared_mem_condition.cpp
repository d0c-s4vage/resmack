#include "resmack/fuzz/ipc/shared_mem_condition.hpp"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>


namespace resmack {
namespace fuzz {
namespace ipc {

  SharedMemCondition::SharedMemCondition(bool shared) : val_(false) {
    Init(shared);
  }

  void SharedMemCondition::Init(bool shared) {
    this->val_ = false;
    pthread_condattr_t attrs;
    pthread_condattr_init(&attrs);
    if (shared) {
      pthread_condattr_setpshared(&attrs, PTHREAD_PROCESS_SHARED);
    }

    pthread_cond_init(&this->condition_, &attrs);
    pthread_condattr_destroy(&attrs);

    this->lock_.Init();
  }

  void SharedMemCondition::Reset() {
    this->lock_.Lock();
    this->val_ = false;
    this->lock_.Unlock();
  }

  void SharedMemCondition::Wait() {
    this->WaitAndHold();
    this->lock_.Unlock();
  }

  void SharedMemCondition::WaitAndHold() {
    this->lock_.Lock();
    this->WaitRaw_Danger();
    // reset the value while we have the lock
    this->val_ = false;
  }

  void SharedMemCondition::WaitRaw_Danger() {
    while (!this->val_) {
      pthread_cond_wait(&this->condition_, &this->lock_.mutex_);
    }
  }

  void SharedMemCondition::Signal() {
    this->lock_.Lock();
    this->SignalRaw_Danger();
    this->lock_.Unlock();
  }

  void SharedMemCondition::SignalRaw_Danger() {
    this->val_ = true;
    pthread_cond_signal(&this->condition_);
  }

  void SharedMemCondition::Lock() {
    this->lock_.Lock();
  }

  void SharedMemCondition::Unlock() {
    this->lock_.Unlock();
  }

}
}
}
