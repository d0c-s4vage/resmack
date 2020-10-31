#include "stdlib.h"
#include "unistd.h"

#include "item.hpp"
#include <cstring>

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
