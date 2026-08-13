#include <limits.h>
#include <cstring>
#include <filesystem>
#include <string>
#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <sys/wait.h>
#include <vector>

#include "resmack/logo.hpp"
#include "resmack/debug.hpp"

namespace resmack {
namespace cli {
namespace compile {

  struct CompileOpts {
    int help;
    int use_asan;
    int debug;
  };

  void PrintHelp() {
    std::cout << GetResmackLogo() << std::endl;

    std::cout << "resmack cc" << std::endl << std::endl;
    std::cout << "  Compile a fuzz harness into a stand-alone binary" << std::endl << std::endl;
    std::cout << "resmack cc CLANG_ARGS" << std::endl << std::endl;
    std::cout << "  --help,-h    Show this help message" << std::endl;
    std::cout << "  --asan,-a    Compile with address sanitizer (ASAN)" << std::endl;
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
      { "help", no_argument, nullptr, 'h' },
      { "asan", no_argument, nullptr, 'a' },
      { "debug", no_argument, nullptr, 'd' },
      { 0, 0, 0, 0 },
    };
    int opt_index = 0;

    while (true) {
      int c = getopt_long(argc, argv, "hsad", long_options, &opt_index);
      if (c == -1) {
        break;
      }

      switch (c) {
        case 'h':
          opts->help = true;
          break;
        case 'a':
          opts->use_asan = true;
          break;
        case 'd':
          opts->debug = true;
          break;
        default:
          // Handle unknown options
          break;
      }
    }

    return true;
  }

  /*
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
  */

  std::string GetExePath() {
    char result[ PATH_MAX ];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return std::string(result, (count > 0) ? count : 0);
  }

  int Run(int argc, char** argv) {
    CompileOpts opts {
      .help = false,
      .use_asan = false,
      .debug = false,
    };

    ParseOpts(argc, argv, &opts);
    if (opts.help) {
      PrintHelp();
      return 1;
    }

    std::vector<std::string> options({
      "clang++",
      "-fno-omit-frame-pointer",
      "-fno-sanitize=all",
      "-Iws/libresmack/include",
    });

    if (opts.debug) {
      options.emplace_back("-g");
    }

    options.emplace_back("-DFINAL");
    options.emplace_back("-DFLAG_USING_ASAN=" + std::to_string(static_cast<int>(opts.use_asan)));
    if (opts.use_asan) {
      options.emplace_back("-O0");
      options.emplace_back("-fsanitize=address");
    } else {
      options.emplace_back("-fno-sanitize=address");
      options.emplace_back("-O3");
      options.emplace_back("-ffast-math");
      options.emplace_back("-march=native");
    }
    options.emplace_back("-fsanitize-coverage=trace-pc-guard");

    for (int curr_opt_ind = optind; curr_opt_ind < argc; curr_opt_ind++) {
      options.emplace_back(argv[curr_opt_ind]);
    }

    options.insert(options.end(), {
      "-Wl,--whole-archive",
      "-lresmack_fuzz_main-static",
      "-Wl,--no-whole-archive",
      "-lresmack_fuzz",
      "-lresmack",
      "-lpthread",
      "-lcrypto",
      "-lunwind",
      "-lunwind-ptrace",
      "-lunwind-generic",
    });

    std::string parent_path = std::filesystem::path(GetExePath()).parent_path();
    options.emplace_back("-L" + parent_path);

    std::cout << "Executing: " << std::endl << std::endl;
    std::vector<char *> argv_c;
    argv_c.reserve(options.size() + 1);
    for (auto& elem : options) {
      std::cout << elem << " ";
      argv_c.push_back(elem.data());
    }
    argv_c.push_back(nullptr);
    std::cout << std::endl << std::endl;

    if (fork() == 0) {
      execvp("clang++", argv_c.data());
    } else {
      wait(NULL);
    }

    std::cout << "Done!" << std::endl;

    return 0;
  }

}
}
}
