#include <cstring>
#include <cstddef>
#include <fcntl.h>
#include <iostream>
#include <semaphore.h>

#include "resmack/fuzz/corpora/mmap.hpp"
#include "resmack/fuzz/ipc_util.hpp"

namespace resmack {
namespace fuzz {
namespace corpora {

MmapCorpus::MmapCorpus() :
  next_item_index(0),
  next_item(NULL)
{
}

MmapCorpus::~MmapCorpus() {}

void MmapCorpus::Init(void* corpus_map, size_t max_corpus_size) {
  this->corpus_map = corpus_map;
  this->max_corpus_size = max_corpus_size;
  this->next_item_index = 0;
  this->next_item = NULL;

  this->meta = (ser::CorpusMetadata*)this->corpus_map;

  if ((this->corpus_lock = sem_open("/resmack-corpus", O_CREAT, 0660, 1)) == SEM_FAILED) {
    perror("Could not create semaphore");
    std::exit(1);
  }

  this->Sync();
}

bool MmapCorpus::AddRandSnapshotIfNotSeen(resmack::Vector<RandSnapshot>* snapshot, size_t feedback_key) {
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

    this->AddRandSnapshotInner(snapshot, feedback_key);
  });

  return res;
}

void MmapCorpus::AddRandSnapshot(resmack::Vector<RandSnapshot>* snapshot, size_t feedback_key) {
  WITH_LOCK(this->corpus_lock, Adding snapshot, {
      this->AddRandSnapshotInner(snapshot, feedback_key);
  });
}

void MmapCorpus::AddRandSnapshotInner(resmack::Vector<RandSnapshot>* snapshot, size_t feedback_key) {
  size_t total_size =
    sizeof(ser::CorpusItemHeader) +
    (sizeof(ser::CorpusRandState) * snapshot->size());
  this->next_item->num_states = snapshot->size();
  this->next_item->size = total_size;
  this->next_item->feedback_key = feedback_key;
  
  ser::CorpusRandState* curr_state = (ser::CorpusRandState*)(
    (char*)this->next_item + sizeof(ser::CorpusItemHeader)
  );
  for (RandSnapshot& item: *snapshot) {
    curr_state->ref_depth = item.ref_depth;
    curr_state->rule_idx = item.rule_idx;
    memcpy(curr_state->rand_state, item.state, sizeof(uint32_t) * 4);

    curr_state = (ser::CorpusRandState*)((char*)curr_state + sizeof(ser::CorpusRandState));
  }

  this->last_updated_seq = ++this->meta->updated_seq;
  this->next_item_index = ++this->meta->num_entries;
  // gets incremented to point at the "next" empty spot after the for loop
  this->next_item = (ser::CorpusItemHeader*)((char*)curr_state);

  this->snapshots.emplace_back(*snapshot);
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
    for (auto snapshot : this->snapshots) {
      snapshot.clear();
    }
    this->snapshots.clear();
    this->next_item_index = 0;
    this->next_item = (ser::CorpusItemHeader*)((char*)this->meta + sizeof(MmapMetadata));
  }

  this->last_updated_seq = this->meta->updated_seq;
  this->last_reorg_seq = this->meta->reorg_seq;

  size_t snapshot_idx = this->next_item_index;
  ser::CorpusItemHeader* curr = this->next_item;

  for(; snapshot_idx < this->meta->num_entries; snapshot_idx++) {
    this->seen_keys.emplace(curr->feedback_key);
    Vector<RandSnapshot>* snapshot = &this->snapshots.emplace_back();

    size_t state_offset = 0;
    size_t header_size = sizeof(ser::CorpusItemHeader);
    size_t state_size = sizeof(ser::CorpusRandState);
    ser::CorpusRandState* state;
    for(size_t rand_state_idx = 0; rand_state_idx < curr->num_states; rand_state_idx++) {
      state = (ser::CorpusRandState*)((char*)curr + header_size + state_offset);
      snapshot->emplace_back(state->ref_depth, state->rule_idx, state->rand_state);
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
    rand_idx = next % corpus_len;
  }
  return &this->snapshots[rand_idx];
}

}
}
}
