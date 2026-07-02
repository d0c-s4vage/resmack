#include <cstdio>
#include <fcntl.h>
#include <stdio.h>
#include <string>
#include <sys/file.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

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
      perror("Could not create semaphore");
      std::exit(1);
    }

    this->anonymous = false;
  }

  Lock::Lock(std::string name, bool shared) : name(name) {
    this->_lock = (sem_t*)malloc(sizeof(sem_t));
    sem_init(this->_lock, shared, 1);
    this->anonymous = true;
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

  bool Lock::Acquire() {
    if (sem_wait(this->_lock) == -1) {
      if (errno == EINTR) {
        return false;
      }
      perror("Could not sem_wait on semaphore");
      std::exit(1);
    }

    return true;
  }

  bool Lock::Release() {
    if (sem_post(this->_lock) == -1) {
      if (errno == EINTR) {
        return false;
      }
      perror("Could not sem_post on semaphore");
      std::exit(1);
    }

    return true;
  }

  void Lock::lock() {
    if (!this->Acquire()) {
      throw std::runtime_error("Could not acquire lock");
    }
  }

  void Lock::unlock() {
    if (!this->Release()) {
      throw std::runtime_error("Could not release lock");
    }
  }

  bool Lock::try_lock() {
    return this->Acquire();
  }

  int Lock::GetValue() {
    int sem_val;
    if (sem_getvalue(this->_lock, &sem_val) == -1) {
      perror("Could not get semaphore value");
      std::exit(1);
    }
    return sem_val;
  }

  void Lock::Init() {}
}
}
