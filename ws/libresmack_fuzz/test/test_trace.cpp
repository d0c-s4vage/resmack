#include <cstdlib>
#include <signal.h>
#include <unistd.h>

#include "gtest/gtest.h"

#include "resmack/defs.hpp"
#include "resmack/fuzz/trace_targets/fork.hpp"
#include "resmack/fuzz/trace.hpp"
#include "resmack/fuzz/tracee.hpp"

namespace resmack {
namespace fuzz {

  TEST(Trace, CatchesCrashes) {
    trace_targets::Fork fork_target(true, [](Tracee* tracee) {
      tracee->SaveLastCorpusInfo(true, 1337, 1338, 1339);
      raise(SIGSEGV);
    });

    Tracer t(
      &fork_target,
      [](pid_t, int, Tracer*, Tracee*) -> bool {
        return false;
      },
      [](pid_t, Tracer*, Tracee*) -> bool { return false; },
      0
    );

    t.Trace();
    t.Join();

    EXPECT_EQ(t.GetCrashInfo()->crashed, true);
    EXPECT_EQ(t.GetCrashInfo()->exit_status, 1);
  }

}
}
