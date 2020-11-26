#include <cstring>
#include <string>
#include <iostream>
#include <unistd.h>
#include "getopt.h"
#include "sys/wait.h"
#include <vector>

#include "resmack/logo.hpp"

namespace resmack {

namespace cli {
namespace compile {

  struct CompileOpts {
    int help;
  };

  void PrintHelp() {
    std::cout << GetResmackLogo() << std::endl;

    std::cout << "resmack cc" << std::endl << std::endl;
    std::cout << "  Compile a fuzz harness into a stand-alone binary" << std::endl << std::endl;
    std::cout << "resmack cc CLANG_ARGS" << std::endl << std::endl;
    std::cout << "   --help, -h    Show this help message" << std::endl;
    std::cout << "   CLANG_ARGS    Arguments to pass to clang (use --" << std::endl;
    std::cout << "                 if needed)" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl << std::endl;
    std::cout << "  resmack cc -- fuzz_target.cpp -o fuzz_target" << std::endl << std::endl;
    std::cout << "resmack cc will set the appropriate clang options for" << std::endl;
    std::cout << "coverage, address sanitizer, and linking with resmack" << std::endl;
    std::cout << "libs" << std::endl;
  }

  bool ParseOpts(int argc, char** argv, CompileOpts* opts) {
    static struct option long_options[] = {
      { "help", no_argument, &opts->help, 'h' },
      { 0, 0, 0, 0 },
    };
    int opt_index = 0;

    while (true) {
      int c = getopt_long(argc, argv, "hs", long_options, &opt_index);
      if (c == -1) {
        break;
      }

      switch (c) {
        case 0:
          break;
        case 'h':
          opts->help = true;
          break;
      }
    }

    return true;
  }

  char* FindLib(const char* lib_name) {
    const char* search_paths[] = {
      "/usr/lib/resmack",
      "/usr/local/lib/resmack",
    };

    // if not found in search paths, maybe we're running from the build
    // directory - find our build folder, and then find the other paths
    //
    // if can't find the lib, print an error message and exit
  }

  int Run(int argc, char** argv) {
    CompileOpts opts {
      .help = false,
    };

    ParseOpts(argc, argv, &opts);
    if (opts.help) {
      PrintHelp();
      return 1;
    }

    std::vector<const char *> options({
      "clang++",
      "-Ofast",
      "-march=native",
      "-lpthread",
      "-fsanitize-coverage=trace-pc-guard",
      //"-fsanitize=address",
    });

    for (int curr_opt_ind = optind; curr_opt_ind < argc; curr_opt_ind++) {
      options.emplace_back(argv[curr_opt_ind]);
    }
    // these go last and are ordered!
    options.emplace_back("build/release/ws/libresmack_fuzz_main/libresmack_fuzz_main.a");
    options.emplace_back("build/release/ws/libresmack_fuzz/libresmack_fuzz.a");
    options.emplace_back("build/release/ws/libresmack/libresmack.a");
    options.emplace_back((const char*)NULL);

    std::cout << "Executing: " << std::endl << std::endl;
    for (size_t i = 0; i < options.size(); i++) {
      if (options[i] == NULL) { continue; }
      std::cout << options[i] << " ";
    }
    std::cout << std::endl << std::endl;

    if (fork() == 0) {
      execvp("clang++", (char* const*)options.data());
    } else {
      wait(NULL);
    }

    std::cout << "Done!" << std::endl;

    return 0;
  }

}
}
}
