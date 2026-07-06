#include <cstdlib>
#include <signal.h>
#include <unistd.h>

#include "gtest/gtest.h"

#include "resmack/debug.hpp"
#include "resmack/fuzz/asan_util.hpp"
#include "resmack/fuzz/trace_targets/fork.hpp"
#include "resmack/fuzz/trace.hpp"
#include "resmack/fuzz/tracee.hpp"

namespace resmack {
namespace fuzz {

  TEST(Trace, CatchesCrashes) {
    bool mute_target = false;
    trace_targets::Fork fork_target(mute_target, [](Tracee* tracee) {
      tracee->SaveLastCorpusInfo(true, 1337, 1338, 1339);

      //raise(SIGSEGV);
      ((void(*)())(0))();
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

    const CrashInfo* info = t.GetCrashInfo();
    EXPECT_EQ(info->crashed, true);
    EXPECT_EQ(info->exit_status, resmack::fuzz::asan::ASAN_EXIT_CODE);
  }

}
}
