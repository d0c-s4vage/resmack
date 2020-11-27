#include <sys/wait.h>
#include <sys/ptrace.h>
#include <unistd.h>

#include "gtest/gtest.h"

#include "resmack/fuzz/tracee.hpp"
#include "resmack/fuzz/trace_target.hpp"
#include "resmack/fuzz/trace_targets/fork.hpp"

namespace resmack {
namespace fuzz {

  TEST(TraceTargetFork, ForksCorrectly) {
    Tracee t;
    trace_targets::Fork fork_target([](Tracee* tracee) {
      tracee->SaveLastCorpusIndex(true, 99999);
    });

    pid_t pid = fork_target.Spawn(&t);
    EXPECT_NE(pid, 0);

    ptrace(PTRACE_DETACH, pid, NULL, NULL);

    int status = 0;
    waitpid(pid, &status, 0);

    EXPECT_EQ(t.GetLastCorpusIndex(), 99999u);
    EXPECT_EQ(t.GetLastUsedCorpus(), true);
  }

}
}
