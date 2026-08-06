#include <fcntl.h>
#include <stdexcept>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <semaphore.h>
#include <sys/file.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>

#include "resmack/debug.hpp"
#include "resmack/fuzz/lock.hpp"
#include "resmack/fuzz/utils.hpp"

namespace resmack {
namespace fuzz {

  Lock::Lock(std::string name) : anonymous(false), name(name) {
    char sem_name[1 + (SHA_DIGEST_LENGTH * 2) + 1]; // SHA_DIGEST_LENGTH + NULL
    sem_name[0] = '/';
    utils::sha1_hex(name.c_str(), name.size(), sem_name);
    this->sem_name.assign(sem_name, strlen(sem_name));

    sem_unlink(this->sem_name.c_str());
    this->_lock = sem_open(this->sem_name.c_str(), O_CREAT | O_EXCL, S_IREAD | S_IWRITE, 1);
    if (this->_lock == SEM_FAILED) {
      throw std::runtime_error(std::string("Could not create new semaphore: ") + std::string(std::strerror(errno)));
    }

    this->anonymous = false;
    this->Init();
  }

  Lock::Lock(std::string name, bool shared) : anonymous(true), name(name) {
    if (shared) {
      this->_lock = static_cast<sem_t*>(mmap(
        NULL,
        sizeof(sem_t),
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
      ));
    } else {
      this->_lock = new sem_t();
    }

    sem_init(this->_lock, shared, 1);
    this->Init();
  }

  Lock::Lock() : Lock("AnonymousLock", true) {}

  Lock::~Lock() {
    if (this->anonymous) {
      sem_destroy(this->_lock);
      munmap(this->_lock, sizeof(sem_t));
      this->_lock = NULL;
    } else {
      sem_unlink(this->sem_name.c_str());
      sem_close(this->_lock);
      this->_lock = NULL;
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
    if (this->GetValue() == 0) {
      this->unlock();
    }
  }
}
}
