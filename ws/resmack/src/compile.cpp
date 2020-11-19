#include <cstring>
#include <string>
#include <iostream>
#include "getopt.h"

#include "banner.hpp"

namespace resmack {
namespace cli {
namespace compile {

  struct CompileOpts {
    int help;
  };

  void PrintHelp() {
    PrintBanner();

    std::cout << "resmack cc" << std::endl << std::endl;
    std::cout << "  Compile a fuzz harness into a stand-alone binary" << std::endl << std::endl;
  }

  bool ParseOpts(int argc, char** argv, CompileOpts* opts) {
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
    CompileOpts opts;

    ParseOpts(argc, argv, &opts);
    if (opts.help) {
      PrintHelp();
      return 1;
    }

    return 0;
  }

}
}
}
