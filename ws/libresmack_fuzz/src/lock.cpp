#include <cstdio>
#include <fcntl.h>
#include <stdio.h>
#include <string>
#include <sys/file.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "resmack/debug.hpp"
#include "resmack/fuzz/lock.hpp"
#include "resmack/fuzz/utils.hpp"

namespace resmack {
namespace fuzz {

  Lock::Lock(std::string name) : name(name) {
    char lock_path[1 + (SHA_DIGEST_LENGTH * 2) + 1]; // SHA_DIGEST_LENGTH + NULL
    lock_path[0] = '/';
    utils::sha1_hex(name.c_str(), name.size(), lock_path);
    this->lock_path.assign(lock_path, strlen(lock_path));

    if ((this->_lock = sem_open(lock_path, O_CREAT, 0600, 1)) == SEM_FAILED) {
      throw std::runtime_error("Could not create new semaphore: " + std::string(std::strerror(errno)));
    }

    this->anonymous = false;
    this->Init();
  }

  Lock::Lock(std::string name, bool shared) : name(name) {
    this->_lock = (sem_t*)malloc(sizeof(sem_t));
    sem_init(this->_lock, shared, 1);
    this->anonymous = true;
    this->Init();
  }

  Lock::Lock() : Lock("AnonymousLock", true) {}

  Lock::~Lock() {
    if (this->anonymous) {
      free(this->_lock);
      this->_lock = NULL;
    } else {
      sem_close(this->_lock);
    }
  }

  void Lock::lock() {
    if (sem_wait(this->_lock) != 0) {
      // when errno is EINTR, it means it was interrupted by a signal handler
      throw std::runtime_error("Could not acquire lock: " + std::string(std::strerror(errno)));
    }
  }

  void Lock::unlock() {
    if (sem_post(this->_lock) != 0) {
      // when errno is EINTR, it means it was interrupted by a signal handler
      throw std::runtime_error("Could not unlock lock: " + std::string(std::strerror(errno)));
    }
  }

  bool Lock::try_lock() {
    if (sem_trywait(this->_lock) == 0) {
      return true;
    }

    if (errno == EAGAIN) {
        return false;
    } else {
      throw std::runtime_error("Could not acquire lock: " + std::string(std::strerror(errno)));
    }
  }

  int Lock::GetValue() {
    int sem_val;
    if (sem_getvalue(this->_lock, &sem_val) == -1) {
      perror("Could not get semaphore value");
      std::exit(1);
    }
    return sem_val;
  }

  void Lock::Init() {
    // just in case a previous process used a named lock and didn't leave it in a good state...
    while (this->GetValue() == 0) {
      this->unlock();
    }
  }
}
}
