#ifndef RESMACK_FUZZ_LOCK_H
#define RESMACK_FUZZ_LOCK_H

#include <string>
#include <unistd.h>
#include <semaphore.h>

namespace resmack {
namespace fuzz {

  class Lock {
   private:
    std::string name;
    sem_t* _lock;
    pid_t creator;
    bool anonymous;
    std::string lock_path;

   public:
    // Create a named semaphore
    Lock(std::string name);
    // Create an anonymous semaphore, with bool specifying if it should be
    // shared.
    Lock(std::string name, bool shared);
    // Create an unnamed, shared semaphore
    Lock();
    ~Lock();

    void lock();
    void unlock();
    bool try_lock();

    int GetValue();

   private:
    void Init();
    void IgnoreSignals();
    void ObserveSignals();
  };

}
}

#endif
