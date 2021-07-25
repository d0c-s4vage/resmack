#include <cstring>
#include <string>
#include <iostream>
#include <getopt.h>

#include "resmack/logo.hpp"

#include "compile.hpp"

namespace resmack {
namespace cli {
namespace root {

  struct MainOpts {
    int help;
  };

  void PrintHelp() {
    std::cout << GetResmackLogo() << std::endl;

    std::cout << "Available sub-commands. Each has their own --help" << std::endl << std::endl;
    std::cout << "  cc           - Compilation" << std::endl;
    //std::cout << "  debug-state  - State file debugging" << std::endl;
    std::cout << "  gen          - Grammar generation" << std::endl;
  }

  bool ParseOpts(int argc, char** argv, MainOpts* opts) {
    static struct option long_options[] = {
      { "help", no_argument, &opts->help, 'h' },
      { 0, 0, 0, 0 },
    };
    int opt_index = 0;

    while (true) {
      int c = getopt_long(argc, argv, "h", long_options, &opt_index);
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

  int Run(int argc, char** argv) {
    MainOpts opts = {
      .help = false,
    };

    bool has_args = argc > 1;

    if (has_args && strncmp(argv[1], "cc", strlen("cc")) == 0) {
      return resmack::cli::compile::Run(argc - 1, &argv[1]);
    //} else if (has_args && strncmp(argv[1], "debug-state", strlen("debug-state")) == 0) {
    //  return resmack::cli::debug_state::Run(argc - 1, &argv[1]);
    }

    if (!ParseOpts(argc, argv, &opts)) {
      std::cerr << "Error: Could not parse command-line arguments" << std::endl;
      return 1;
    }

    if (argc == 1 || opts.help || optind == argc) {
      PrintHelp();
      return 1;
    }

    return 0;
  }

}
}
}

int main(int argc, char** argv) {
  return resmack::cli::root::Run(argc, argv);
}
