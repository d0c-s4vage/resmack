#include <cstdlib>
#include <signal.h>
#include <unistd.h>

#include "gtest/gtest.h"

#include "resmack/debug.hpp"
#include "resmack/fuzz/asan_util.hpp"
#include "resmack/fuzz/process_launchers/fork.hpp"
#include "resmack/fuzz/tracer.hpp"
#include "resmack/fuzz/tracee.hpp"

namespace resmack {
namespace fuzz {

  TEST(Trace, CatchesCrashes) {
    bool mute_target = false;
    process_launchers::ForkLauncher fork_target(mute_target, [](Tracee* tracee) {
      tracee->SaveLastCorpusInfo(true, 1337, 1338, 1339);

      //raise(SIGSEGV);
      ((void(*)())(0))();
    });

    Tracer t(
      &fork_target,
      [](pid_t, Tracer* _t, Tracee*) -> void {
        // stop on the first crash
        _t->Stop(false);
        return;
      },
      [](pid_t, Tracer*, Tracee*) -> void { return; },
      0
    );

    t.Start();
    t.Join();

    const CrashInfo* info = t.GetCrashInfo();
    EXPECT_EQ(info->crashed, true);
    EXPECT_EQ(info->signal_info.exit_status, resmack::fuzz::asan::ASAN_EXIT_CODE);
  }

}
}
