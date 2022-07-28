#include <stdio.h>

#include "resmack/fuzz/states/mmap.hpp"
#include "resmack/fuzz/cmds/debug_state.hpp"

namespace resmack {
namespace fuzz {
namespace cmds {

  void DebugCorpusEntry(char** curr_entry) {
    ser::CorpusItemHeader* header = (ser::CorpusItemHeader*)*curr_entry;
    printf("    size:                       %zu\n", header->size);
    printf("    feedback_key:               0x%zx\n", header->feedback_key);
    printf("    feedback_num:               %zu\n", header->feedback_num);
    printf("    iter_discovered:            %zu\n", header->iter_discovered);
    printf("    parent1_one_based_idx:      %zu\n", header->parent1_one_based_idx);
    printf("    parent2_one_based_idx:      %zu\n", header->parent2_one_based_idx);
    printf("    mutations_since_offspring:  %zu\n", header->mutations_since_offspring);
    printf("    num_crashes:                %zu\n", header->num_crashes);
    printf("    num_ancestors:              %zu\n", header->num_ancestors);
    printf("    num_descendants:            %zu\n", header->num_descendants);
    printf("    num_direct_descendants:     %zu\n", header->num_direct_descendants);
    printf("    reserved1:                  %zu\n", header->reserved1);
    printf("    reserved2:                  %zu\n", header->reserved2);
    printf("    item_header:\n");
    printf("      num_states: %zu\n", header->item_header.num_states);

    ser::GenState* curr = (ser::GenState*)(header + 1);
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
    ser::CorpusMetadata* meta = (ser::CorpusMetadata*)corpus_data;
    printf("Corpus Metadata:\n");
    printf("  updated_seq: %u\n", meta->updated_seq);
    printf("  reorg_seq:   %u\n", meta->reorg_seq);
    printf("  num_entries: %u\n", meta->num_entries);

    char* curr_ptr = corpus_data + sizeof(ser::CorpusMetadata);
    for(size_t i = 0; i < meta->num_entries; i++) {
      printf("  entry[%lu]:\n", i);
      DebugCorpusEntry(&curr_ptr);
    }
  }

  void DebugStateData(char* state_data) {
    states::StateMetadata* meta = (states::StateMetadata*)state_data;
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

    DebugCorpus(state_data + sizeof(states::StateMetadata));
  }

  void DebugState(DebugStateConfig* config) {
    struct stat stat_info;
    if (stat(config->state_path, &stat_info) == -1) {
      perror("Error fetching info about state file");
      return;
    }

    char* file_contents = (char*)malloc(stat_info.st_size);
    printf("State file size: %lx\n", stat_info.st_size);
    
    FILE* fd = fopen(config->state_path, "rb");
    if (fd == NULL) {
      perror("Error opening file");
      return;
    }

    if (fread(file_contents, 1, stat_info.st_size, fd) != (size_t)stat_info.st_size) {
      printf("Could not read full file\n");
      return;
    }

    DebugStateData(file_contents);
    free(file_contents);
  }

}
}
}
