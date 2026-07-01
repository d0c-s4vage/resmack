#include <algorithm>
#include <cstring>
#include <cstddef>
#include <fcntl.h>
#include <semaphore.h>

#include "resmack/debug.hpp"

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
    next_item(NULL),
    corpus_decay(1000000) // 1 million iters with no new offspring == 0 priority
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
    this->last_item1_one_based_idx = 0;
    this->last_item2_one_based_idx = 0;

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
      DEBUG_PRINT("%d: SyncInner()\n", getpid());
      this->SyncInner();
      DEBUG_PRINT("%d: Done SyncInner()\n", getpid());

      if (this->SeenFeedback(stats.key)) {
        DEBUG_PRINT("%d: Already saw this feedback\n", getpid());
        res = false;
        break;
      }

      DEBUG_PRINT("%d: AddRandSnapshotInner()\n", getpid());
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
    this->next_item->mutations_since_offspring = new_snapshot->mutations_since_offspring;
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
      curr_state->max_depth = item.max_depth;
      curr_state->rule_idx = item.rule_idx;
      memcpy(curr_state->rand_state, item.state, sizeof(uint32_t) * 4);

      curr_state++;
      new_snapshot->snapshot.emplace_back(
        item.ref_depth,
        item.max_depth,
        item.rule_idx,
        item.state
      );
    }

    this->last_updated_seq = ++this->meta->updated_seq;
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
      entry.mutations_since_offspring = curr->mutations_since_offspring;
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
        entry.snapshot.emplace_back(
          state->ref_depth,
          state->max_depth,
          state->rule_idx,
          state->rand_state
        );
        state_offset += state_size;
      }

      curr = (ser::CorpusItemHeader*)((char*)curr + header_size + state_offset);
    }

    this->next_item_index = snapshot_idx;
    this->next_item = curr;

    this->SyncCountersInner();
    this->SortedsResort();
  }

  // Update all mutations_since_offspring counters. Assumed to be only called
  // from within an IPC-safe context (from within a WITH_LOCK block)
  void MmapCorpus::SyncCounters() {
    if (this->NumItems() != this->meta->num_entries) {
      this->Sync();
      return;
    }

    WITH_LOCK(this->corpus_lock, Syncing Counters, {
      this->SyncCountersInner();
    });
    this->SortedsResort();
  }

  void MmapCorpus::SyncCountersInner() {
    size_t header_size = sizeof(ser::CorpusItemHeader);
    size_t state_size = sizeof(ser::GenState);

    ser::CorpusItemHeader* curr = this->first_item;
    size_t snapshot_idx = 0;
    for (; snapshot_idx < this->snapshots.size(); snapshot_idx++) {
      CorpusEntry& entry = this->snapshots[snapshot_idx];
      if (~((uint64_t)0) - curr->mutations_since_offspring < entry.mutations_since_offspring_new) {
        curr->mutations_since_offspring = ~((uint64_t)0);
      } else {
        curr->mutations_since_offspring += entry.mutations_since_offspring_new;
      }
      entry.mutations_since_offspring_new = 0;
      entry.mutations_since_offspring = curr->mutations_since_offspring;
      entry.decay_pct =
        (float)(this->corpus_decay - entry.mutations_since_offspring) /
        (float)this->corpus_decay;
      curr = (ser::CorpusItemHeader*)((char*)curr + header_size + (state_size * curr->item_header.num_states));
    }
  }

  float MmapCorpus::GetDecayPercent() {
    float total = 0.0;

    size_t header_size = sizeof(ser::CorpusItemHeader);
    size_t state_size = sizeof(ser::GenState);

    ser::CorpusItemHeader* curr = this->first_item;
    size_t snapshot_idx = 0;
    size_t num_items = this->meta->num_entries;
    for (; snapshot_idx < num_items; snapshot_idx++) {
      size_t num = curr->mutations_since_offspring > this->corpus_decay ?
        this->corpus_decay :
        curr->mutations_since_offspring;
      float pct = (float)(this->corpus_decay - num) / (float)this->corpus_decay;
      total += pct;
      curr = (ser::CorpusItemHeader*)((char*)curr + header_size + (state_size * curr->item_header.num_states));
    }

    return total / (float)num_items * 100.0;
  }

  void MmapCorpus::SetStrats(uint32_t strats) {
    this->strats = strats;

    if (STRAT_RAND & strats) {
      this->strat_handlers.push_back(this->HandleRandStrat);
    }
    if (STRAT_MOST_FEEDBACK & strats) {
      this->strat_handlers.push_back(this->HandleMostFeedbackStrat);
    }
    if (STRAT_LEAST_FEEDBACK & strats) {
      this->strat_handlers.push_back(this->HandleLeastFeedbackStrat);
    }
    if (STRAT_MOST_RECENT & strats) {
      this->strat_handlers.push_back(this->HandleMostRecentStrat);
    }
    if (STRAT_LEAST_RECENT & strats) {
      this->strat_handlers.push_back(this->HandleLeastRecentStrat);
    }
    if (STRAT_MOST_ANCESTORS & strats) {
      this->strat_handlers.push_back(this->HandleMostAncestorsStrat);
    }
    if (STRAT_LEAST_ANCESTORS & strats) {
      this->strat_handlers.push_back(this->HandleLeastAncestorsStrat);
    }
    if (STRAT_MOST_DIRECT_DESCENDANTS & strats) {
      this->strat_handlers.push_back(this->HandleMostDirectDescendantsStrat);
    }
    if (STRAT_LEAST_DIRECT_DESCENDANTS & strats) {
      this->strat_handlers.push_back(this->HandleLeastDirectDescendantsStrat);
    }
    if (STRAT_MOST_DESCENDANTS & strats) {
      this->strat_handlers.push_back(this->HandleMostDescendantsStrat);
    }
    if (STRAT_LEAST_DESCENDANTS & strats) {
      this->strat_handlers.push_back(this->HandleLeastDescendantsStrat);
    }
  }

  size_t MmapCorpus::HandleRandStrat(MmapCorpus* this_, Rand* rand, size_t) {
    size_t corpus_len = this_->snapshots.size();
    return rand->Next() % corpus_len;
  }
  size_t MmapCorpus::HandleMostFeedbackStrat(MmapCorpus* this_, Rand*, size_t rand_top_ten) {
    return this_->most_feedback[rand_top_ten];
  }
  size_t MmapCorpus::HandleLeastFeedbackStrat(MmapCorpus* this_, Rand*, size_t rand_top_ten) {
    return this_->most_feedback[this_->most_feedback.size() - rand_top_ten - 1];
  }
  size_t MmapCorpus::HandleMostRecentStrat(MmapCorpus* this_, Rand*, size_t rand_top_ten) {
    return this_->snapshots.size() - rand_top_ten - 1;
  }
  size_t MmapCorpus::HandleLeastRecentStrat(MmapCorpus*, Rand*, size_t rand_top_ten) {
    return rand_top_ten;
  }
  size_t MmapCorpus::HandleMostAncestorsStrat(MmapCorpus* this_, Rand*, size_t rand_top_ten) {
    return this_->most_ancestors_desc[rand_top_ten];
  }
  size_t MmapCorpus::HandleLeastAncestorsStrat(MmapCorpus* this_, Rand*, size_t rand_top_ten) {
    return this_->most_ancestors_desc[this_->most_ancestors_desc.size() - rand_top_ten - 1];
  }
  size_t MmapCorpus::HandleMostDirectDescendantsStrat(MmapCorpus* this_, Rand*, size_t rand_top_ten) {
    return this_->most_direct_descendants_desc[rand_top_ten];
  }
  size_t MmapCorpus::HandleLeastDirectDescendantsStrat(MmapCorpus* this_, Rand*, size_t rand_top_ten) {
    return this_->most_direct_descendants_desc[this_->most_direct_descendants_desc.size() - rand_top_ten - 1];
  }
  size_t MmapCorpus::HandleMostDescendantsStrat(MmapCorpus* this_, Rand*, size_t rand_top_ten) {
    return this_->most_descendants_desc[rand_top_ten];
  }
  size_t MmapCorpus::HandleLeastDescendantsStrat(MmapCorpus* this_, Rand*, size_t rand_top_ten) {
    return this_->most_descendants_desc[this_->most_descendants_desc.size() - rand_top_ten - 1];
  }

  Vector<RandSnapshot>* MmapCorpus::GetItem(Rand* rand, size_t* last_idx1, size_t* last_idx2) {
    if (this->strat_handlers.size() == 0) {
      this->SetStrats(STRAT_RAND | STRAT_MOST_RECENT | STRAT_LEAST_DIRECT_DESCENDANTS);
    }

    this->Sync();

    uint32_t choice_val = rand->Next();
    uint32_t rand_val = rand->Next();
    size_t corpus_len = this->snapshots.size();
    size_t top_ten = corpus_len >= 10 ? 10 : corpus_len;
    size_t rand_top_ten = rand_val % top_ten;

    size_t rand_idx = this->strat_handlers[choice_val % this->strat_handlers.size()](
      this, rand, rand_top_ten
    );

    *last_idx1 = this->last_item1_one_based_idx = rand_idx + 1;
    *last_idx2 = this->last_item2_one_based_idx = 0;

    CorpusEntry* entry = &this->snapshots[rand_idx];
    entry->mutations_since_offspring_new++;
    entry->decay_pct =
      (float)(this->corpus_decay - entry->mutations_since_offspring - entry->mutations_since_offspring_new) /
      (float)this->corpus_decay;

    return &entry->snapshot;
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
      if (parent == entry) {
        DEBUG_PRINT("%d: PARENT WAS ENTRY! idx: %zu\n", getpid(), parent_idx);
        break;
      }
      ser::CorpusItemHeader* parent_header = this->GetItemHeader(parent_idx);
      if (level == 0) {
        // a new offspring was found! reset back to 0
        parent->mutations_since_offspring = 0;
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

  void MmapCorpus::IncUnwanted(size_t one_based_idx) {
    if (one_based_idx == 0) { return; }

    DEBUG_PRINT("   idx: %lu - Incrementing unwanted\n", one_based_idx);
    WITH_LOCK(this->corpus_lock, Incrementing Unwanted Idx, {
      DEBUG_PRINT("   idx: %lu - Getting item header\n", one_based_idx);
      ser::CorpusItemHeader* header = this->GetItemHeader(one_based_idx - 1);

      //DEBUG_PRINT("   idx: %lu - Incrementing snapshot\n", one_based_idx);
      //this->snapshots[one_based_idx - 1].mutations_since_offspring += 5000;

      DEBUG_PRINT("   idx: %lu - bumping mutations since offspring\n", one_based_idx);
      header->mutations_since_offspring += 5000;

      DEBUG_PRINT("   idx: %lu - Incrementing last_updated_seq\n", one_based_idx);
      this->last_updated_seq = ++this->meta->updated_seq;
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
    this->most_feedback.push_back(index);
  }

  void MmapCorpus::SortedsClear() {
    this->most_direct_descendants_desc.clear();
    this->most_descendants_desc.clear();
    this->most_ancestors_desc.clear();
    this->most_feedback.clear();
  }

  void MmapCorpus::SortedsResort() {
    Vector<CorpusEntry>* snapshots = &this->snapshots;

    std::sort(
      this->most_direct_descendants_desc.begin(),
      this->most_direct_descendants_desc.end(),
      [snapshots](size_t left_idx, size_t right_idx) -> bool {
        CorpusEntry& left = snapshots->at(left_idx);
        CorpusEntry& right = snapshots->at(right_idx);

        return left.decay_pct * left.num_direct_descendants >
          right.decay_pct * right.num_direct_descendants;
      }
    );

    std::sort(
      this->most_descendants_desc.begin(),
      this->most_descendants_desc.end(),
      [snapshots](size_t left_idx, size_t right_idx) -> bool {
        CorpusEntry& left = snapshots->at(left_idx);
        CorpusEntry& right = snapshots->at(right_idx);

        return left.decay_pct * left.num_descendants >
          right.decay_pct * right.num_descendants;
      }
    );

    std::sort(
      this->most_ancestors_desc.begin(),
      this->most_ancestors_desc.end(),
      [snapshots](size_t left_idx, size_t right_idx) -> bool {
        CorpusEntry& left = snapshots->at(left_idx);
        CorpusEntry& right = snapshots->at(right_idx);

        return left.decay_pct * left.num_ancestors >
          right.decay_pct * right.num_ancestors;
      }
    );

    std::sort(
      this->most_feedback.begin(),
      this->most_feedback.end(),
      [snapshots](size_t left_idx, size_t right_idx) -> bool {
        CorpusEntry& left = snapshots->at(left_idx);
        CorpusEntry& right = snapshots->at(right_idx);

        return left.decay_pct * left.feedback_num >
          right.decay_pct * right.feedback_num;
      }
    );
  }

}
}
}
