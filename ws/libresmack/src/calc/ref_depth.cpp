#include "ref_depth.hpp"

namespace resmack {
namespace calc {

  RefDepth::RefDepth(RuleManager* rule_man): rule_man_(rule_man) {}
  RefDepth::~RefDepth() {}

  void RefDepth::Calc() {
    size_t before_options = 0;
    bool changed = false;

    this->num_changes_ = 0;

    Vector<items::Or*>* rules = this->rule_man_->GetRules();
    do {
      changed = false;
      this->tmp_to_prune_.clear();

      for (size_t rule_idx = 0; rule_idx < rules->size(); rule_idx++) {
        items::Or* rule = (*rules)[rule_idx];
        if (NULL == rule) { continue; }
        // it has been finalized and officially pruned, so ignore it
        if (this->pruned_.contains(rule_idx)) { continue; }

        std::string name;
        this->rule_man_->NameOf(rule_idx, &name);

        before_options = rule->NumShortestItems();
        size_t depth = rule->CalcRefDepth(this);
        if (!this->depths_.contains(rule_idx) || this->depths_[rule_idx] != depth) {
          changed = true;
        }
          
        this->depths_[rule_idx] = depth;

        if (before_options != rule->NumShortestItems()) {
          changed = true;
        }

        if (!changed && depth == RefDepth::INF_DEPTH && !rule->ShouldKeep()) {
          this->tmp_to_prune_.emplace(rule_idx);
          this->num_changes_ += 1;
        }
      }
    } while(changed);

    for (size_t rule_idx: this->tmp_to_prune_) {
      this->rule_man_->Prune(rule_idx);
    }
  }

  size_t RefDepth::DepthOf(std::string rule_name) {
    size_t rule_idx;
    if (!this->rule_man_->IndexOf(rule_name, &rule_idx)) {
      return this->INF_DEPTH;
    }
    if (this->depths_.contains(rule_idx)) {
      return this->depths_[rule_idx];
    }
    return this->INF_DEPTH;
  }

  size_t RefDepth::CalcItem(Item *item) {
    return item->CalcRefDepth(this);
  }

}
}
