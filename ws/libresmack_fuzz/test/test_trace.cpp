#include <unistd.h>

#include "gtest/gtest.h"

#include "resmack/fuzz/trace.hpp"

namespace resmack {
namespace fuzz {

  TEST(Trace, CatchesCrashes) {
    /*
    Trace t;

    pid_t pid;
    if ((pid = fork()) == 0) {
      t.Traceme();
    }

    t.Trace(pid);
    */
  }

}
}
