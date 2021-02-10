#ifndef RESMACK_FUZZ_TARGET_NEW_H
#define RESMACK_FUZZ_TARGET_NEW_H

#include <string>
#include <unistd.h>

namespace resmack {
namespace fuzz {
namespace targets {

  class Target {
   private:
   public:
    virtual pid_t Start() = 0;
    virtual int Test(const std::string* input) = 0;
    virtual void Stop() = 0;
  };

}
}
}

#endif
