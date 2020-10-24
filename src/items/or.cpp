#include <cstdarg>
#include <cstddef>
#include <limits>

#include "types.hpp"
#include "../item.hpp"
#include "or.hpp"
#include "raw.hpp"

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
    size_t chosen_idx;
    Vector<size_t> *choice_list;

    if (this->keep_) {
      chosen_idx = (ctx->rand->Next() % this->items_.size());
    } else {
      size_t choice_idx;
      if (ctx->DoShortest() && this->shortest_indices_.size() > 0) {
        choice_list = &this->shortest_indices_;
      } else {
        choice_list = &this->choice_indices_;
      }

      if (choice_list->size() == 1) {
        choice_idx = 0;
      } else {
        choice_idx = (ctx->rand->Next() % choice_list->size());
      }
      chosen_idx = (*choice_list)[choice_idx];
    }

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

  Or* Or::AddItems(const char* first, .../*Item* item, .., NULL sentinel value*/) {
    va_list args;
    va_start(args, first);
    const char* next = first;

    while (NULL != next) {
      if (next == NULL) { break; }
      this->items_.push_back(new items::Raw(std::string(next)));
      next = va_arg(args, const char*);
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
    return this->keep_ || this->choice_indices_.size() != 0;
  }

  size_t Or::CalcRefDepth(calc::RefDepth* ref_depth) {
    // will only happen for run-time added rule values with PreId, which
    // will only be Raw values, not complex types with additional refs
    if (this->keep_) {
      return 0;
    }

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

  std::string Or::ToString() {
    std::string res = "<OR";

    res += " choices=[";
    for (auto idx: this->choice_indices_) {
      res += std::to_string(idx) + ",";
    }
    if (this->choice_indices_.size() > 0) {
      res[res.size()-1] = ']';
    } else {
      res += "]";
    }

    res += " shortest=[";
    for (auto idx: this->shortest_indices_) {
      res += std::to_string(idx) + ",";
    }
    if (this->shortest_indices_.size() > 0) {
      res[res.size()-1] = ']';
    } else {
      res += "]";
    }

    res += " items=[";
    for (auto item: this->items_) {
      res += item->ToString() + ",";
    }
    if (this->items_.size() > 0) {
      res[res.size()-1] = ']';
    } else {
      res += "]";
    }

    return res + ">";
  }

}
}
