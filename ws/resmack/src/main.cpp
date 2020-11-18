#include <cstring>
#include <string>
#include <iostream>
#include "getopt.h"

namespace resmack {
namespace cli {
namespace root {

  struct MainOpts {
    int help;
  };

  void PrintHelp() {
    const char* banner = "reSMACK";
    std::cout << banner << std::endl << std::endl;

    std::cout << "Sub-Commands:" << std::endl << std::endl;
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

    if (!ParseOpts(argc, argv, &opts)) {
      std::cerr << "Error: Could not parse command-line arguments" << std::endl;
      return 1;
    }

    if (argc == 1 || opts.help || optind == argc) {
      PrintHelp();
      return 1;
    }

    std::string sub_command(argv[optind]);

    if (sub_command == "cc") {
      std::cerr << "Compiling" << std::endl;
    } else {
      std::cerr << "Unknown sub-command: " << sub_command << std::endl << std::endl;
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
