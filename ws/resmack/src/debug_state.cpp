#include <getopt.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/wait.h>

#include "resmack/logo.hpp"
#include "resmack/fuzz/serialized.hpp"
#include "resmack/fuzz/state.hpp"
#include "resmack/fuzz/states/mmap.hpp"
#include "resmack/fuzz/corpus.hpp"
#include "resmack/fuzz/corpora/mmap.hpp"

namespace resmack {
namespace cli {
namespace debug_state {

  struct DebugStateOpts {
    int help;
  };

  void PrintHelp() {
    std::cout << GetResmackLogo() << std::endl;

    std::cout << "resmack debug-state" << std::endl << std::endl;
    std::cout << "  Debug persisted state information" << std::endl << std::endl;
    std::cout << "resmack debug-state STATE_FILE" << std::endl << std::endl;
    std::cout << "    --help,-h             Show this help message" << std::endl;
    std::cout << "              STATE_FILE  The state file to debug" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl << std::endl;
    std::cout << "  resmack debug-state a.out.resmack-state" << std::endl << std::endl;
  }

  bool ParseOpts(int argc, char** argv, DebugStateOpts* opts) {
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

  void DebugCorpusEntry(char** curr_entry) {
    fuzz::ser::CorpusItemHeader* header = (fuzz::ser::CorpusItemHeader*)*curr_entry;
    printf("    size:                   %zu\n", header->size);
    printf("    feedback_key:           0x%zx\n", header->feedback_key);
    printf("    feedback_num:           %zu\n", header->feedback_num);
    printf("    iter_discovered:        %zu\n", header->iter_discovered);
    printf("    parent1_one_based_idx:  %zu\n", header->parent1_one_based_idx);
    printf("    parent2_one_based_idx:  %zu\n", header->parent2_one_based_idx);
    printf("    num_crashes:            %zu\n", header->num_crashes);
    printf("    num_ancestors:          %zu\n", header->num_ancestors);
    printf("    num_descendants:        %zu\n", header->num_descendants);
    printf("    num_direct_descendants: %zu\n", header->num_direct_descendants);
    printf("    reserved1:              %zu\n", header->reserved1);
    printf("    reserved2:              %zu\n", header->reserved2);
    printf("    item_header:\n");
    printf("      num_states: %zu\n", header->item_header.num_states);

    fuzz::ser::GenState* curr = (fuzz::ser::GenState*)(header + 1);
    for (size_t i = 0; i < header->item_header.num_states; i++) {
      printf("        state[%3lu]: ref_depth: %5u max_depth: %5u rule_idx: %5u rand_state: %08x|%08x|%08x|%08x\n",
        i,
        curr->ref_depth,
        curr->max_depth,
        curr->rule_idx,
        curr->rand_state[0],
        curr->rand_state[1],
        curr->rand_state[2],
        curr->rand_state[3]
      );
      curr++;
    }

    *curr_entry = (char*)curr;
  }

  void DebugCorpus(char* corpus_data) {
    fuzz::ser::CorpusMetadata* meta = (fuzz::ser::CorpusMetadata*)corpus_data;
    printf("Corpus Metadata:\n");
    printf("  updated_seq: %u\n", meta->updated_seq);
    printf("  reorg_seq:   %u\n", meta->reorg_seq);
    printf("  num_entries: %u\n", meta->num_entries);

    char* curr_ptr = corpus_data + sizeof(fuzz::ser::CorpusMetadata);
    for(size_t i = 0; i < meta->num_entries; i++) {
      printf("  entry[%lu]:\n", i);
      DebugCorpusEntry(&curr_ptr);
    }
  }

  void DebugState(char* state_data) {
    fuzz::states::StateMetadata* meta = (fuzz::states::StateMetadata*)state_data;
    printf("State Metadata:\n");
    printf("  iterations: %lu\n", meta->iterations);
    printf("  crashes:    %lu\n", meta->crashes);
    printf("  stats:\n");
#define STAT(NAME) printf("    %-14s%-15f\n", #NAME":", meta->stats.duration_##NAME);
#include "resmack/fuzz/stats.def"
#undef STAT
    printf("  reserved1:  %lu\n", meta->reserved1);
    printf("  reserved2:  %lu\n", meta->reserved2);
    printf("  reserved3:  %lu\n", meta->reserved3);
    printf("  reserved4:  %lu\n", meta->reserved4);
    printf("  reserved5:  %lu\n", meta->reserved5);
    printf("  reserved6:  %lu\n", meta->reserved6);
    printf("  reserved7:  %lu\n", meta->reserved7);
    printf("  reserved8:  %lu\n", meta->reserved8);

    DebugCorpus(state_data + sizeof(fuzz::states::StateMetadata));
  }

  int Run(int argc, char** argv) {
    DebugStateOpts opts {
      .help = false,
    };

    ParseOpts(argc, argv, &opts);
    if (opts.help) {
      PrintHelp();
      return 1;
    }

    if (argv[optind] == NULL) {
      std::cout << "Must provide the STATE_FILE\n" << std::endl;
      PrintHelp();
      return 1;
    }

    const char* state_file = argv[optind];
    struct stat stat_info;
    if (stat(state_file, &stat_info) == -1) {
      perror("Error fetching info about state file");
      return 1;
    }

    char* file_contents = (char*)malloc(stat_info.st_size);
    printf("State file size: %lx\n", stat_info.st_size);
    
    FILE* fd = fopen(state_file, "rb");
    if (fd == NULL) {
      perror("Error opening file");
      return 1;
    }

    if (fread(file_contents, 1, stat_info.st_size, fd) != (size_t)stat_info.st_size) {
      printf("Could not read full file\n");
      return 1;
    }

    DebugState(file_contents);

    free(file_contents);

    return 0;
  }

}
}
}
