#include <cstring>
#include <cstddef>
#include <fcntl.h>
#include <iostream>
#include <semaphore.h>

#include "resmack/fuzz/corpora/mmap.hpp"
#include "resmack/fuzz/ipc_util.hpp"
#include "resmack/fuzz/utils.hpp"

namespace resmack {
namespace fuzz {
namespace corpora {

MmapCorpus::MmapCorpus() :
  next_item_index(0),
  first_item(NULL),
  next_item(NULL)
{
}

MmapCorpus::~MmapCorpus() {
}

void MmapCorpus::Init(
  const char* state_path,
  void* corpus_map,
  size_t max_corpus_size
) {
  this->corpus_map = corpus_map;
  this->max_corpus_size = max_corpus_size;
  this->next_item_index = 0;
  this->first_item = NULL;
  this->next_item = NULL;

  this->meta = (ser::CorpusMetadata*)this->corpus_map;

  const char* suffix = "-corpus";
  size_t suffix_len = strlen(suffix);
  size_t state_len = strlen(state_path);
  char* with_sem = (char*)malloc(state_len + suffix_len + 1);
  memcpy(with_sem, state_path, state_len);
  // keep the null terminator
  memcpy(with_sem + state_len, suffix, suffix_len + 1);

  char sem_path[2 + (SHA_DIGEST_LENGTH * 2)]; // leading '/' + SHA_DIGEST_LENGTH + NULL
  utils::sha1_hex(with_sem, strlen(with_sem), sem_path+1);
  sem_path[0] = '/';

  free(with_sem);

  if ((this->corpus_lock = sem_open(sem_path, O_CREAT, 0660, 1)) == SEM_FAILED) {
    perror("Could not create semaphore");
    std::exit(1);
  }

  // *ALWAYS* sync when in Init (don't use Sync())
  WITH_LOCK(this->corpus_lock, Syncing corpus, {
    this->SyncInner();
  });
}

bool MmapCorpus::AddRandSnapshotIfNotSeen(
  const resmack::Vector<RandSnapshot>* snapshot,
  size_t feedback_key,
  bool descendant_of_last
) {
  bool res = true;
  if (this->SeenFeedback(feedback_key)) {
    return false;
  }

  WITH_LOCK(this->corpus_lock, Maybe adding snapshot, {
    this->SyncInner();

    if (this->SeenFeedback(feedback_key)) {
      res = false;
      break;
    }

    this->AddRandSnapshotInner(snapshot, feedback_key, descendant_of_last);
  });

  return res;
}

void MmapCorpus::AddRandSnapshot(
  const resmack::Vector<RandSnapshot>* snapshot,
  size_t feedback_key,
  bool descendant_of_last
) {
  WITH_LOCK(this->corpus_lock, Adding snapshot, {
      this->AddRandSnapshotInner(snapshot, feedback_key, descendant_of_last);
  });
}

void MmapCorpus::AddRandSnapshotInner(
  const resmack::Vector<RandSnapshot>* snapshot,
  size_t feedback_key,
  bool descendant_of_last
) {
  this->last_discovered_iteration = *this->curr_iter_count;

  CorpusEntry* new_snapshot = &this->snapshots.emplace_back();
  new_snapshot->index = this->snapshots.size() - 1;
  if (descendant_of_last) {
    new_snapshot->parent1 = this->last_item1;
    new_snapshot->parent2 = this->last_item2;
    new_snapshot->num_ancestors = this->UpdateStats(new_snapshot, 0);
  }

  size_t total_size =
    sizeof(ser::CorpusItemHeader) +
    (sizeof(ser::GenState) * snapshot->size());

  this->next_item->item_header.num_states = snapshot->size();
  this->next_item->size = total_size;
  this->next_item->feedback_key = feedback_key;
  this->next_item->iter_discovered = this->last_discovered_iteration;
  this->next_item->num_ancestors = new_snapshot->num_ancestors;
  this->next_item->num_direct_descendants = new_snapshot->num_direct_descendants;
  this->next_item->num_descendants = new_snapshot->num_descendants;
  this->next_item->num_crashes = new_snapshot->num_crashes;

  if (new_snapshot->parent1 != NULL) {
    this->next_item->parent_1_one_based_idx = new_snapshot->parent1->index + 1;
  }
  if (new_snapshot->parent2 != NULL) {
    this->next_item->parent_2_one_based_idx = new_snapshot->parent2->index + 1;
  }
  
  ser::GenState* curr_state = (ser::GenState*)(
    (char*)this->next_item + sizeof(ser::CorpusItemHeader)
  );

  for (const RandSnapshot& item: *snapshot) {
    curr_state->ref_depth = item.ref_depth;
    curr_state->rule_idx = item.rule_idx;
    memcpy(curr_state->rand_state, item.state, sizeof(uint32_t) * 4);

    curr_state = (ser::GenState*)((char*)curr_state + sizeof(ser::GenState));
    new_snapshot->snapshot.emplace_back(item.ref_depth, item.rule_idx, item.state);
  }

  this->last_updated_seq = ++this->meta->updated_seq;
  this->next_item_index = ++this->meta->num_entries;
  // gets incremented to point at the "next" empty spot after the for loop
  this->next_item = (ser::CorpusItemHeader*)((char*)curr_state);
  this->seen_keys.emplace(feedback_key);
}

void MmapCorpus::Sync() {
  // nothing has been changed, bail
  if (this->meta->updated_seq == this->last_updated_seq) {
    return;
  }

  WITH_LOCK(this->corpus_lock, Syncing corpus, {
    this->SyncInner();
  });
}

void MmapCorpus::SyncInner() {
  if (this->next_item == NULL || this->meta->reorg_seq != this->last_reorg_seq) {
    for (auto entry : this->snapshots) {
      entry.snapshot.clear();
    }
    this->snapshots.clear();
    this->next_item_index = 0;
    this->next_item =
      (ser::CorpusItemHeader*)((char*)this->meta + sizeof(ser::CorpusMetadata));
    this->first_item = this->next_item;
  }

  this->last_updated_seq = this->meta->updated_seq;
  this->last_reorg_seq = this->meta->reorg_seq;

  size_t snapshot_idx = this->next_item_index;
  ser::CorpusItemHeader* curr = this->next_item;

  for(; snapshot_idx < this->meta->num_entries; snapshot_idx++) {
    this->seen_keys.emplace(curr->feedback_key);
    CorpusEntry& entry = this->snapshots.emplace_back();
    if (curr->parent_1_one_based_idx != 0) {
      entry.parent1 = &this->snapshots[curr->parent_1_one_based_idx];
    }
    if (curr->parent_2_one_based_idx != 0) {
      entry.parent2 = &this->snapshots[curr->parent_1_one_based_idx];
    }
    entry.num_crashes = curr->num_crashes;
    entry.num_descendants = curr->num_descendants;
    entry.num_direct_descendants = curr->num_direct_descendants;
    entry.num_ancestors = curr->num_ancestors;
    entry.iter_discovered = curr->iter_discovered;

    size_t state_offset = 0;
    size_t header_size = sizeof(ser::CorpusItemHeader);
    size_t state_size = sizeof(ser::GenState);
    ser::GenState* state;

    for(
      size_t rand_state_idx = 0;
      rand_state_idx < curr->item_header.num_states;
      rand_state_idx++
    ) {
      state = (ser::GenState*)((char*)curr + header_size + state_offset);
      entry.snapshot.emplace_back(state->ref_depth, state->rule_idx, state->rand_state);
      state_offset += state_size;
    }

    curr = (ser::CorpusItemHeader*)((char*)curr + header_size + state_offset);
  }

  this->next_item_index = snapshot_idx;
  this->next_item = curr;
}

Vector<RandSnapshot>* MmapCorpus::GetItem(Rand* rand) {
  size_t corpus_len = this->snapshots.size();
  size_t rand_idx;

  if (corpus_len > 4) {
    size_t half_size = corpus_len / 2;
    size_t first_half = corpus_len - half_size;
    // double the odds of the last half
    size_t new_size = corpus_len + half_size;
    size_t tmp = rand->Next() % new_size;
    if (tmp < first_half) {
      rand_idx = tmp;
    } else {
      rand_idx = first_half + (tmp - first_half) / 2;
    }
  } else {
    uint32_t next = rand->Next();
    printf("CORPUS LENGTH: %lu\n", corpus_len);
    rand_idx = next % corpus_len;
  }

  // TODO crossover

  this->last_item1 = &this->snapshots[rand_idx];
  this->last_item2 = NULL;
  return &this->last_item1->snapshot;
}

// returns the maximum ancestor depth max(ancestor_parent1, ancestor_parent2)
size_t MmapCorpus::UpdateStats(CorpusEntry* entry, size_t level) {
  if (entry->parent1 == NULL && entry->parent2 == NULL) {
    return 0;
  }

  CorpusEntry* parents[2] = { entry->parent1, entry->parent2 };
  size_t max_level = 0;

  for (size_t i = 0; i < 2; i++) {
    CorpusEntry* parent = parents[i];
    if (parent == NULL) { continue; }

    ser::CorpusItemHeader* parent_header = this->GetItemHeader(parent->index);
    if (level == 0) {
      parent->num_direct_descendants++;
      parent_header->num_direct_descendants++;
    }
    size_t parent_level = this->UpdateStats(parent, level+1);
    // will already be set! num ancestors never changes
    // parent->num_ancestors++;
    parent->num_descendants++;
    parent_header->num_descendants++;
    if (parent_level > max_level) {
      max_level = parent_level;
    }
  }

  return max_level;
}

void MmapCorpus::IncLastItemCrashes() {
  WITH_LOCK(this->corpus_lock, Incrementing Last Item Crashes, {
    this->last_item1->num_crashes++;
    this->GetItemHeader(this->last_item1->index)->num_crashes++;

    if (this->last_item2 != NULL) {
      this->last_item2->num_crashes++;
      this->GetItemHeader(this->last_item2->index)->num_crashes++;
    }
  });
}

ser::CorpusItemHeader* MmapCorpus::GetItemHeader(size_t index) {
  ser::CorpusItemHeader* curr = this->first_item;
  for (size_t i = 0; i < index; i++) {
    curr = (ser::CorpusItemHeader*)((char*)curr + curr->size + sizeof(ser::CorpusItemHeader));
  }
  return curr;
}

}
}
}
