#include <sys/wait.h>
#include <sys/ptrace.h>
#include <unistd.h>

#include "gtest/gtest.h"

#include "resmack/fuzz/tracee.hpp"
#include "resmack/fuzz/process_launcher.hpp"
#include "resmack/fuzz/process_launchers/fork.hpp"

namespace resmack {
namespace fuzz {

  TEST(ProcessLauncherFork, ForksCorrectly) {
    Tracee t(0);
    process_launchers::ForkLauncher fork_target(false, [](Tracee* tracee) {
      tracee->SaveLastCorpusInfo(true, 99999, 88888, 77777);
    });

    pid_t pid = fork_target.Spawn(&t);
    EXPECT_NE(pid, 0);

    ptrace(PTRACE_DETACH, pid, nullptr, nullptr);

    int status = 0;
    int wait_res = waitpid(pid, &status, 0);

    EXPECT_EQ(wait_res, pid) << "Could not waitpid, wait() result: " << wait_res << ", error" << strerror(errno);

    EXPECT_EQ(t.GetLastCorpusIndex1(), 99999u);
    EXPECT_EQ(t.GetLastCorpusIndex2(), 88888u);
    EXPECT_EQ(t.GetLastMaxDepth(), 77777u);
    EXPECT_EQ(t.GetLastUsedCorpus(), true);
  }

}
}
