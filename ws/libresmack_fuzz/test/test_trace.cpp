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
      tracee->SaveLastCorpusInfo(true, 1337, 1338);
      raise(SIGSEGV);
    });

    Tracer t(
      &fork_target,
      [](pid_t pid, int status, Tracer* tracer, Tracee* tracee) -> bool {
        UNUSED(pid); UNUSED(status); UNUSED(tracer); UNUSED(tracee);

        return false;
      }
    );

    t.Trace();
    t.Join();

    EXPECT_EQ(t.GetCrashInfo()->signal, SIGSEGV);
  }

}
}
