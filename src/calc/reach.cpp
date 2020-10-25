#include "reach.hpp"

namespace resmack {
namespace calc {

  Reach::Reach(RuleManager* rule_man): rule_man_(rule_man), num_changes_(0) {}
  Reach::~Reach() {}

  void Reach::Calc() {
    this->num_changes_ = 0;
    this->tmp_to_prune_.clear();
    this->tmp_new_rules_.clear();

    Vector<items::Or*>* rules = this->rule_man_->GetRules();
    for (size_t rule_idx = 0; rule_idx < rules->size(); rule_idx++) {
      items::Or* rule = (*rules)[rule_idx];
      if (NULL == rule) { continue; }
      // it has been finalized and officially pruned, so ignore it
      if (this->pruned_.contains(rule_idx)) { continue; }

      if (!this->CalcItem(rule)) {
        this->tmp_to_prune_.emplace(rule_idx);
      }
    }

    for (auto rule_idx: this->tmp_to_prune_) {
      std::string rule_name;
      if (!this->rule_man_->NameOf(rule_idx, &rule_name)) { continue; }

      this->rule_man_->Prune(rule_idx);
      this->pruned_.emplace(rule_idx);
      this->num_changes_++;
    }
  }

  size_t Reach::NumChanges() {
    return this->num_changes_;
  }

  bool Reach::CalcItem(Item* item) {
    bool fully_reachable = item->CalcReachability(this);

    switch(item->Type()) {
      case ItemType::REF:
        if (!fully_reachable) {
          this->unresolved_refs_.emplace(((items::Ref*)item)->rule_name_);
        }
        break;
      default:
        break;
    }

    return fully_reachable;
  }

  bool Reach::IndexOf(std::string rule_name, size_t* out) {
    return this->rule_man_->IndexOf(rule_name, out);
  }

  void Reach::Ensure(std::string rule_name, size_t* out) {
    this->tmp_new_rules_.emplace(rule_name);

    size_t size_before = this->rule_man_->NumRules();
    items::Or* or_ = this->rule_man_->Ensure(rule_name);
    or_->SetKeep(true);
    this->rule_man_->IndexOf(rule_name, out);

    if (size_before != this->rule_man_->NumRules()) {
      this->num_changes_++;
    }
  }

}
}
