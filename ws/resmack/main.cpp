#include "stdlib.h"
#include "unistd.h"

#include "item.hpp"
#include <cstring>

void fuzz() {
  Rules* rules;
  LoadGrammar(&rules, ...);

  Feedback feedback;
  Target target;
  Rand rand(100);

  Corpus corpus = NULL;

  std::string output;
  output.reserve(0x1000);

  size_t start_rule_idx = rules.GetRuleMan()->IndexOf("main");
  size_t max_ref_depth = 10;
  BuildContext ctx(&output, &rand, max_ref_depth);

  while (true) {
    rand.SnapshotClear();
    if (rand.Maybe()) {
      ctx.SetReplay(corpus.Fetch(&rand));
    } else {
      // build a new one from scratch
      ctx.SetReplay(NULL);
    }
    rules.Build(start_rule_idx, &ctx);

    target.Launch(&output, &feedback);

    FeedbackStats stats = feedback.GetStats();
    if (corpus.IsNew(&stats) {
      corpus.Save(stats, rand.GetSnapshots());
    }
  }
}

int main(int argc, char** argv) {
  // cc:
  //   * forward all arguments to clang, adding
  //     * -fsanitize=coverage... arguments
  //     * linking with libresmack_.a
  //     * linking with libresmack_fuzz_main.a
  //   * report on successful compilation
  //
  // gen:
  //   * -i input type (json, webidl, antlr)
  //   * -o output type (json, cpp)
  //
  // fuzz:
  //   * -t target binary to fuzz
  //   * -f fuzz type - snapshot, etc.
  //   * -g grammar path
  //   * -T grammar type - json, cpp (cpp is compiled with a new main and then run)
  //   * -- ARGS - args to the target program, replacing {INPUT} with input file path?
  std::cout << "ARGC: " << argc << ", ARGV:" << std::endl;
  if (argc < 2) {
    std::cout << "Usage" << std::endl;
    return 1;
  }

  if (strcmp(argv[1], "cc") == 0 && argc > 2) {
    std::cout << "COMPILING" << std::endl;
    char* to_compile = argv[2];
    const char* clang_argv[] = {
      "-fsanitize-coverage=trace-pc-guard,trace-cmp,trace-div,indirect-calls",
      "-lresmack_fuzz",
      "-o",
      "output",
      to_compile,
      NULL,
    };
    execv("/usr/bin/clang++", (char* const*)clang_argv);
  }
}
