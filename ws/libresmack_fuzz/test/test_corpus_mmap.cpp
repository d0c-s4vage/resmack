#include <cstdio>
#include <cstring>
#include <sys/mman.h>

#include "gtest/gtest.h"

#include "resmack/rand.hpp"
#include "resmack/fuzz/corpus.hpp"
#include "resmack/fuzz/corpora/mmap.hpp"
#include "resmack/fuzz/serialized.hpp"

namespace resmack {
namespace fuzz {

  TEST(MmapCorpus, AddsItemsCorrectly) {
    Rand rand;
    rand.SetShouldRecord(true);
    rand.Next();
    rand.SnapshotState(1, 5, 100);
    rand.Next();
    rand.SnapshotState(2, 5, 200);

    size_t curr_iter_count = 0;
    size_t map_size = 0x1000;
    void* map;

    if((map = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
      ASSERT_EQ(true, false);
    }
    memset(map, 0, map_size);

    corpora::MmapCorpus corpus;
    corpus.SetCurrIterPtr(&curr_iter_count);
    corpus.Init("resmack-test", map, map_size);
    EXPECT_EQ(corpus.NumItems(), 0u);

    Vector<RandSnapshot> snapshots(*rand.GetSnapshots());

    pid_t pid;
    if (!(pid = fork())) {
      corpus.AddRandSnapshot(
        &snapshots,
        { .new_coverage = false, .key = 0, .num = 1 },
        false
      );
      std::exit(1);
    } else {
      int status;
      waitpid(pid, &status, 0);
    }

    ser::CorpusMetadata* meta = (ser::CorpusMetadata*)map;

    EXPECT_EQ(meta->updated_seq, 1u);
    EXPECT_EQ(meta->reorg_seq, 0u);
    EXPECT_EQ(meta->num_entries, 1u);

    EXPECT_EQ(corpus.NumItems(), 0u);
    corpus.Sync();
    EXPECT_EQ(corpus.NumItems(), 1u);

    Vector<RandSnapshot>* fetchedSnapshots = corpus.GetItem(&rand);
    EXPECT_EQ(fetchedSnapshots->size(), 2u);
    EXPECT_EQ(fetchedSnapshots->size(), snapshots.size());

    EXPECT_NE(fetchedSnapshots, &snapshots);

    EXPECT_EQ((*fetchedSnapshots)[0].ref_depth, 1u);
    EXPECT_EQ((*fetchedSnapshots)[0].ref_depth, snapshots[0].ref_depth);
    EXPECT_EQ((*fetchedSnapshots)[0].rule_idx, 100u);
    EXPECT_EQ((*fetchedSnapshots)[0].rule_idx, snapshots[0].rule_idx);

    EXPECT_EQ((*fetchedSnapshots)[1].ref_depth, 2u);
    EXPECT_EQ((*fetchedSnapshots)[1].ref_depth, snapshots[1].ref_depth);
    EXPECT_EQ((*fetchedSnapshots)[1].rule_idx, 200u);
    EXPECT_EQ((*fetchedSnapshots)[1].rule_idx, snapshots[1].rule_idx);

    EXPECT_NE((*fetchedSnapshots)[0].state[0], 0u);
    EXPECT_EQ((*fetchedSnapshots)[0].state[0], snapshots[0].state[0]);
    EXPECT_NE((*fetchedSnapshots)[0].state[1], 0u);
    EXPECT_EQ((*fetchedSnapshots)[0].state[1], snapshots[0].state[1]);
    EXPECT_NE((*fetchedSnapshots)[0].state[2], 0u);
    EXPECT_EQ((*fetchedSnapshots)[0].state[2], snapshots[0].state[2]);
    EXPECT_NE((*fetchedSnapshots)[0].state[3], 0u);
    EXPECT_EQ((*fetchedSnapshots)[0].state[3], snapshots[0].state[3]);

    EXPECT_NE((*fetchedSnapshots)[1].state[0], 0u);
    EXPECT_EQ((*fetchedSnapshots)[1].state[0], snapshots[1].state[0]);
    EXPECT_NE((*fetchedSnapshots)[1].state[1], 0u);
    EXPECT_EQ((*fetchedSnapshots)[1].state[1], snapshots[1].state[1]);
    EXPECT_NE((*fetchedSnapshots)[1].state[2], 0u);
    EXPECT_EQ((*fetchedSnapshots)[1].state[2], snapshots[1].state[2]);
    EXPECT_NE((*fetchedSnapshots)[1].state[3], 0u);
    EXPECT_EQ((*fetchedSnapshots)[1].state[3], snapshots[1].state[3]);

    munmap(map, map_size);
  }

  TEST(MmapCorpus, MultipleSyncs) {
    Rand rand;
    rand.SetShouldRecord(true);
    rand.Next();
    rand.SnapshotState(1, 5, 100);
    rand.Next();
    rand.SnapshotState(2, 5, 200);

    size_t iteration_count = 0;

    size_t map_size = 0x1000;
    void* map;

    if((map = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
      ASSERT_EQ(true, false);
    }
    memset(map, 0, map_size);

    corpora::MmapCorpus corpus;
    corpus.SetCurrIterPtr(&iteration_count);
    corpus.Init("resmack-test-2", map, map_size);
    EXPECT_EQ(corpus.NumItems(), 0u);

    Vector<RandSnapshot> snapshots(*rand.GetSnapshots());

    pid_t pid;
    if (!(pid = fork())) {
      corpus.AddRandSnapshot(
        &snapshots,
        { .new_coverage = false, .key = 0, .num = 1 },
        false
      );
      std::exit(1);
    } else {
      int status;
      waitpid(pid, &status, 0);
    }

    ser::CorpusMetadata* meta = (ser::CorpusMetadata*)map;

    EXPECT_EQ(meta->updated_seq, 1u);
    EXPECT_EQ(meta->reorg_seq, 0u);
    EXPECT_EQ(meta->num_entries, 1u);

    EXPECT_EQ(corpus.NumItems(), 0u);
    corpus.Sync();
    EXPECT_EQ(corpus.NumItems(), 1u);
    corpus.Sync();
    EXPECT_EQ(corpus.NumItems(), 1u);
    corpus.Sync();
    EXPECT_EQ(corpus.NumItems(), 1u);

    munmap(map, map_size);
  }

  TEST(MmapCorpus, MultipleSyncsWithUpdate) {
    Rand rand;
    rand.SetShouldRecord(true);
    rand.Next();
    rand.SnapshotState(1, 5, 100);
    rand.Next();
    rand.SnapshotState(2, 5, 200);

    size_t map_size = 0x1000;
    size_t total_count_ptr = 0;
    void* map;

    if((map = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
      ASSERT_EQ(true, false);
    }
    memset(map, 0, map_size);

    corpora::MmapCorpus corpus;
    corpus.Init("resmack-test-3", map, map_size);
    corpus.SetCurrIterPtr(&total_count_ptr);
    EXPECT_EQ(corpus.NumItems(), 0u);

    Vector<RandSnapshot> snapshots(*rand.GetSnapshots());

    pid_t pid;
    if (!(pid = fork())) {
      corpus.AddRandSnapshot(
        &snapshots,
        { .new_coverage = false, .key = 0, .num = 1 },
        false
      );
      std::exit(1);
    } else {
      int status;
      waitpid(pid, &status, 0);
    }

    ser::CorpusMetadata* meta = (ser::CorpusMetadata*)map;

    EXPECT_EQ(meta->updated_seq, 1u);
    EXPECT_EQ(meta->reorg_seq, 0u);
    EXPECT_EQ(meta->num_entries, 1u);

    EXPECT_EQ(corpus.NumItems(), 0u);
    corpus.Sync();
    EXPECT_EQ(corpus.NumItems(), 1u);
    corpus.Sync();
    EXPECT_EQ(corpus.NumItems(), 1u);


    // fork again, add another snapshot (same one)
    if (!(pid = fork())) {
      corpus.AddRandSnapshot(
        &snapshots,
        { .new_coverage = false, .key = 0, .num = 1 },
        false
      );
      std::exit(1);
    } else {
      int status;
      waitpid(pid, &status, 0);
    }

    corpus.Sync();
    EXPECT_EQ(corpus.NumItems(), 2u);

    const Vector<CorpusEntry>* items = corpus.GetItems();
    for (size_t i = 0; i < items->size(); i++) {
      const CorpusEntry* entry = &items->at(i);
      EXPECT_EQ(entry->parent1_one_based_idx, 0u);
      EXPECT_EQ(entry->parent2_one_based_idx, 0u);
      EXPECT_EQ(entry->index, i);
      EXPECT_EQ(entry->iter_discovered, 0u);
      EXPECT_EQ(entry->num_ancestors, 0u);
      EXPECT_EQ(entry->num_direct_descendants, 0u);
      EXPECT_EQ(entry->num_descendants, 0u);
      EXPECT_EQ(entry->num_crashes, 0u);

      EXPECT_EQ(entry->snapshot.size(), 2u);
    }

    munmap(map, map_size);
  }
}
}
