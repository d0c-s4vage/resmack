#include <cstdarg>
#include <cstddef>
#include <limits>

#include "types.hpp"
#include "../item.hpp"
#include "or.hpp"

namespace resmack {
namespace items {

  Or::Or() : Or(false) {}
  Or::Or(bool keep): keep_(keep) {}
  Or::~Or() {
    for (Item *item: this->items_) {
      delete item;
    }
    this->items_.clear();
  }

  ItemType Or::Type() {
    return ItemType::OR;
  }

  void Or::Build(BuildContext* ctx) {
    size_t choice_idx;
    if (this->choice_indices_.size() == 1) {
      choice_idx = 0;
    } else {
      choice_idx = (ctx->rand->Next() % this->choice_indices_.size());
    }
    size_t chosen_idx = this->choice_indices_[choice_idx];
    this->items_[chosen_idx]->Build(ctx);
  }

  Or* Or::AddItem(Item *item) {
    this->items_.push_back(item);
    return this;
  }

  Or* Or::AddItems(Item* first, .../*Item* item, .., NULL sentinel value*/) {
    va_list args;
    va_start(args, first);
    Item* next = first;

    while (NULL != next) {
      if (next == NULL) { break; }
      this->items_.push_back(next);
      next = va_arg(args, Item*);
    }
    return this;
  }

  bool Or::CalcReachability(calc::Reach* reach_calc) {
    this->choice_indices_.clear();

    for (size_t idx = 0; idx < this->items_.size(); idx++) {
      Item* item = this->items_[idx];
      if (item->CalcReachability(reach_calc)) {
        this->choice_indices_.emplace_back(idx);
      }
    }

    // none of our options are reachable!
    return this->choice_indices_.size() != 0;
  }

  size_t Or::CalcRefDepth(calc::RefDepth* ref_depth) {
    this->shortest_indices_.clear();
    size_t shortest_len = std::numeric_limits<size_t>::max();
    Map<size_t, size_t> depths;

    for (size_t idx = 0; idx < this->choice_indices_.size(); idx++) {
      Item* item = this->items_[this->choice_indices_[idx]];
      size_t len = item->CalcRefDepth(ref_depth);
      depths[idx] = len;
      if (len <= shortest_len) {
        shortest_len = len;
      }
    }

    for (auto it = depths.begin(); it != depths.end(); it++) {
      size_t idx = it->first;
      size_t depth = it->second;

      if (depth != shortest_len) { continue; }

      this->shortest_indices_.emplace_back(idx);
    }

    return shortest_len;
  }

}
}
