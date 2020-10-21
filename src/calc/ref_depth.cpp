#include "ref_depth.hpp"

namespace resmack {
namespace calc {

  RefDepth::RefDepth(Map<std::string, items::Or*>* map): map_(map) {}
  RefDepth::~RefDepth() {}

  void RefDepth::Calc() {
    size_t before_options = 0;
    bool changed = false;

    this->num_changes_ = 0;

    do {
      changed = false;
      this->tmp_to_prune_.clear();

      for (auto it = this->map_->begin(); it != this->map_->end(); it++) {
        std::string rule_name = it->first;
        items::Or* rule_or = it->second;
        // it has been finalized and officially pruned, so ignore it
        if (this->pruned_.contains(rule_name)) { continue; }

        before_options = rule_or->NumShortestItems();
        size_t depth = rule_or->CalcRefDepth(this);
        this->depths_[rule_name] = depth;

        if (before_options != rule_or->NumShortestItems()) {
          changed = true;
        }

        if (depth == RefDepth::INF_DEPTH && !rule_or->ShouldKeep()) {
          this->tmp_to_prune_.emplace(rule_name);
          this->num_changes_ += 1;
        }
      }
    } while(changed);

    for (auto rule_name: this->tmp_to_prune_) {
      this->map_->erase(rule_name);
    }
  }

  size_t RefDepth::DepthOf(std::string rule_name) {
    if (this->depths_.contains(rule_name)) {
      return this->depths_[rule_name];
    }
    return this->INF_DEPTH;
  }

  size_t RefDepth::CalcItem(Item *item) {
    return item->CalcRefDepth(this);
  }

}
}
