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

    if ((this->lock = sem_open(lock_path, O_CREAT, 0600, 1)) == SEM_FAILED) {
      perror("Could not create semaphore");
      std::exit(1);
    }

    this->anonymous = false;
  }

  Lock::Lock(std::string name, bool shared) : name(name) {
    this->lock = (sem_t*)malloc(sizeof(sem_t));
    sem_init(this->lock, shared, 1);
    this->anonymous = true;
  }

  Lock::Lock() : Lock("AnonymousLock", true) {}

  Lock::~Lock() {
    if (this->anonymous) {
      free(this->lock);
      this->lock = NULL;
    } else {
      sem_close(this->lock);
    }
  }

  bool Lock::Acquire() {
    if (sem_wait(this->lock) == -1) {
      if (errno == EINTR) {
        return false;
      }
      perror("Could not sem_wait on semaphore");
      std::exit(1);
    }

    return true;
  }

  bool Lock::Release() {
    if (sem_post(this->lock) == -1) {
      if (errno == EINTR) {
        return false;
      }
      perror("Could not sem_post on semaphore");
      std::exit(1);
    }

    return true;
  }

  int Lock::GetValue() {
    int sem_val;
    if (sem_getvalue(this->lock, &sem_val) == -1) {
      perror("Could not get semaphore value");
      std::exit(1);
    }
    return sem_val;
  }

  void Lock::Init() {}
}
}
