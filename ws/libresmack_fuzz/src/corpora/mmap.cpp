#include <algorithm>
#include <cstring>
#include <cstddef>
#include <fcntl.h>
#include <iostream>
#include <semaphore.h>

#include "resmack/fuzz/feedback.hpp"
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
  FeedbackStats stats,
  bool descendant_of_last
) {
  bool res = true;
  if (this->SeenFeedback(stats.key)) {
    return false;
  }

  WITH_LOCK(this->corpus_lock, Maybe adding snapshot, {
    this->SyncInner();

    if (this->SeenFeedback(stats.key)) {
      res = false;
      break;
    }

    this->AddRandSnapshotInner(snapshot, stats, descendant_of_last);
  });

  return res;
}

void MmapCorpus::AddRandSnapshot(
  const resmack::Vector<RandSnapshot>* snapshot,
  FeedbackStats stats,
  bool descendant_of_last
) {
  WITH_LOCK(this->corpus_lock, Adding snapshot, {
    this->SyncInner();
    this->AddRandSnapshotInner(snapshot, stats, descendant_of_last);
  });
}

void MmapCorpus::AddRandSnapshotInner(
  const resmack::Vector<RandSnapshot>* snapshot,
  FeedbackStats stats,
  bool descendant_of_last
) {
  this->last_discovered_iteration = *this->curr_iter_count;

  CorpusEntry* new_snapshot = &this->snapshots.emplace_back();
  new_snapshot->index = this->snapshots.size() - 1;
  new_snapshot->feedback_key = stats.key;
  new_snapshot->feedback_num = stats.num;

  if (descendant_of_last) {
    new_snapshot->parent1_one_based_idx = this->last_item1_one_based_idx;
    new_snapshot->parent2_one_based_idx = this->last_item2_one_based_idx;
    new_snapshot->num_ancestors = this->UpdateStats(new_snapshot, 0);
  } else {
    new_snapshot->parent1_one_based_idx = 0;
    new_snapshot->parent2_one_based_idx = 0;
  }

  // Need to do this *AFTER* we have updated any stats!
  this->SortedsAdd(new_snapshot->index);
  this->SortedsResort();

  size_t total_size =
    sizeof(ser::CorpusItemHeader) +
    (sizeof(ser::GenState) * snapshot->size());

  this->next_item->size = total_size;
  this->next_item->feedback_key = stats.key;
  this->next_item->feedback_num = stats.num;
  this->next_item->iter_discovered = this->last_discovered_iteration;
  this->next_item->num_crashes = new_snapshot->num_crashes;
  this->next_item->num_ancestors = new_snapshot->num_ancestors;
  this->next_item->num_direct_descendants = new_snapshot->num_direct_descendants;
  this->next_item->num_descendants = new_snapshot->num_descendants;
  this->next_item->item_header.num_states = snapshot->size();
  this->next_item->parent1_one_based_idx = new_snapshot->parent1_one_based_idx;
  this->next_item->parent2_one_based_idx = new_snapshot->parent2_one_based_idx;
  
  ser::GenState* curr_state = (ser::GenState*)(
    (char*)this->next_item + sizeof(ser::CorpusItemHeader)
  );

  for (const RandSnapshot& item: *snapshot) {
    curr_state->ref_depth = item.ref_depth;
    curr_state->rule_idx = item.rule_idx;
    memcpy(curr_state->rand_state, item.state, sizeof(uint32_t) * 4);

    curr_state++;
    new_snapshot->snapshot.emplace_back(item.ref_depth, item.rule_idx, item.state);
  }

  this->last_updated_seq = this->last_updated_seq = ++this->meta->updated_seq;
  this->next_item_index = ++this->meta->num_entries;
  // gets incremented to point at the "next" empty spot after the for loop
  this->next_item = (ser::CorpusItemHeader*)((char*)curr_state);
  this->seen_keys.emplace(stats.key);
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
    this->SortedsClear();
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
    entry.index = snapshot_idx;
    entry.feedback_key = curr->feedback_key;
    entry.feedback_num = curr->feedback_num;
    this->SortedsAdd(snapshot_idx);

    if (curr->parent1_one_based_idx != 0) {
      entry.parent1_one_based_idx = curr->parent1_one_based_idx;
    }
    if (curr->parent2_one_based_idx != 0) {
      entry.parent2_one_based_idx = curr->parent2_one_based_idx;
    }
    entry.iter_discovered = curr->iter_discovered;
    entry.num_ancestors = curr->num_ancestors;
    entry.num_direct_descendants = curr->num_direct_descendants;
    entry.num_descendants = curr->num_descendants;
    entry.num_crashes = curr->num_crashes;

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

  this->SortedsResort();
}

Vector<RandSnapshot>* MmapCorpus::GetItem(Rand* rand) {
  this->Sync();

  size_t corpus_len = this->snapshots.size();
  size_t rand_idx;

  uint32_t choice_val = rand->Next();
  uint32_t rand_val = rand->Next();
  size_t top_fourth = corpus_len / 4;
  size_t top_ten = corpus_len >= 10 ? 10 : corpus_len;
  size_t rand_top_ten = rand_val % top_ten;
  size_t total_options = 7;

  if (corpus_len < 4) {
    total_options = 1; // only the first case statement
  } else if (
      choice_val % 7 == 6 &&
      this->snapshots[this->most_crashes_desc[0]].num_crashes == 0
  ) {
    total_options = 6; // don't use the crashes, that's the same as rand
  }

  switch (choice_val % total_options) {
    // random
    case 0: {
      rand_idx = rand->Next() % corpus_len;
      break;
    }

    // pick one of the most recent ten items
    case 1: {
      rand_idx = corpus_len - rand_top_ten - 1;
      break;
    }

    // top 4th of largest ancestor chain
    case 2: {
      rand_idx = this->most_ancestors_desc[rand_top_ten];
      break;
    }

    // top 4th of most direct descendants
    case 3: {
      rand_idx = this->most_direct_descendants_desc[rand_top_ten];
      break;
    }

    // top 4th of most total descendants
    case 4: {
      rand_idx = this->most_descendants_desc[rand_top_ten];
      break;
    }

    case 5: {
      rand_idx = this->most_feedback[rand_top_ten];
      break;
    }

    // top 4th of most crashes
    case 6: {
      rand_idx = this->most_crashes_desc[rand_top_ten];
      break;
    }
  };

  this->last_item1_one_based_idx = rand_idx + 1;
  this->last_item2_one_based_idx = 0;

  return &(this->snapshots[rand_idx].snapshot);
}

// returns the maximum ancestor depth max(ancestor_parent1, ancestor_parent2)
size_t MmapCorpus::UpdateStats(CorpusEntry* entry, size_t level) {
  if (entry->parent1_one_based_idx == 0 && entry->parent2_one_based_idx == 0) {
    return 0;
  }

  size_t parents[2] = {
    entry->parent1_one_based_idx,
    entry->parent2_one_based_idx,
  };
  size_t max_level = 0;

  for (size_t i = 0; i < 2; i++) {
    size_t parent_idx = parents[i];
    if (parent_idx == 0) { continue; }
    parent_idx -= 1;

    CorpusEntry* parent = &this->snapshots[parent_idx];
    ser::CorpusItemHeader* parent_header = this->GetItemHeader(parent_idx);
    if (level == 0) {
      parent->num_direct_descendants++;
      parent_header->num_direct_descendants++;
    }
    // will already be set! num ancestors never changes
    // parent->num_ancestors++;
    parent->num_descendants++;
    parent_header->num_descendants++;

    size_t parent_level = this->UpdateStats(parent, level+1);
    if (parent_level > max_level) {
      max_level = parent_level;
    }
  }

  return max_level + 1;
}

void MmapCorpus::IncLastItemCrashes() {
  WITH_LOCK(this->corpus_lock, Incrementing Last Item Crashes, {
    this->snapshots[this->last_item1_one_based_idx - 1].num_crashes++;
    this->GetItemHeader(this->last_item1_one_based_idx - 1)->num_crashes++;

    if (this->last_item2_one_based_idx != 0) {
      this->snapshots[this->last_item2_one_based_idx - 1].num_crashes++;
      this->GetItemHeader(this->last_item2_one_based_idx - 1)->num_crashes++;
    }
  });
}

ser::CorpusItemHeader* MmapCorpus::GetItemHeader(size_t index) {
  ser::CorpusItemHeader* curr = this->first_item;
  for (size_t i = 0; i < index; i++) {
    curr = (ser::CorpusItemHeader*)((char*)curr + curr->size);
  }
  return curr;
}

void MmapCorpus::SortedsAdd(size_t index) {
  this->most_direct_descendants_desc.push_back(index);
  this->most_descendants_desc.push_back(index);
  this->most_ancestors_desc.push_back(index);
  this->most_crashes_desc.push_back(index);
  this->most_feedback.push_back(index);
}

void MmapCorpus::SortedsClear() {
  this->most_direct_descendants_desc.clear();
  this->most_descendants_desc.clear();
  this->most_ancestors_desc.clear();
  this->most_crashes_desc.clear();
  this->most_feedback.clear();
}

void MmapCorpus::SortedsResort() {
  Vector<CorpusEntry>* snapshots = &this->snapshots;

  std::sort(
    this->most_direct_descendants_desc.begin(),
    this->most_direct_descendants_desc.end(),
    [snapshots](size_t left_idx, size_t right_idx) -> bool {
      return (*snapshots)[left_idx].num_direct_descendants >
        (*snapshots)[right_idx].num_direct_descendants;
    }
  );

  std::sort(
    this->most_descendants_desc.begin(),
    this->most_descendants_desc.end(),
    [snapshots](size_t left_idx, size_t right_idx) -> bool {
      return (*snapshots)[left_idx].num_descendants >
        (*snapshots)[right_idx].num_descendants;
    }
  );

  std::sort(
    this->most_ancestors_desc.begin(),
    this->most_ancestors_desc.end(),
    [snapshots](size_t left_idx, size_t right_idx) -> bool {
      return (*snapshots)[left_idx].num_ancestors >
        (*snapshots)[right_idx].num_ancestors;
    }
  );

  std::sort(
    this->most_crashes_desc.begin(),
    this->most_crashes_desc.end(),
    [snapshots](size_t left_idx, size_t right_idx) -> bool {
      return (*snapshots)[left_idx].num_crashes >
        (*snapshots)[right_idx].num_crashes;
    }
  );

  std::sort(
    this->most_feedback.begin(),
    this->most_feedback.end(),
    [snapshots](size_t left_idx, size_t right_idx) -> bool {
      return (*snapshots)[left_idx].feedback_num >
        (*snapshots)[right_idx].feedback_num;
    }
  );
}

}
}
}
