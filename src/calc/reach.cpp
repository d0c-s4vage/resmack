#include "reach.hpp"

namespace resmack {
namespace calc {

  Reach::Reach(std::map<std::string, items::Or*>* map): map_(map) {}
  Reach::~Reach() {}

  void Reach::Calc() {
    this->tmp_new_rules_.clear();
    this->tmp_to_prune_.clear();

    for (auto it = this->map_->begin(); it != this->map_->end(); it++) {
      std::string rule_name = it->first;
      items::Or* rule_or = it->second;
      // it has been finalized, officially pruned
      if (this->pruned_->contains(rule_name)) { continue; }

      this->CalcItem(rule_or);
    }

    for (auto rule_name: this->tmp_to_prune_) {
      if (this->tmp_to_prune_.contains(rule_name)) { continue; }

      this->map_->erase(rule_name);
      this->pruned_->emplace(rule_name);
    }

    for (auto rule_name: this->tmp_new_rules_) {
      this->map_->emplace(rule_name, new items::Or(true));
    }
  }

  size_t Reach::NumChanges() {
    return 0;
  }

  bool Reach::CalcItem(Item* item) {
    bool fully_reachable = item->CalcReachability(this);

    switch(item->Type()) {
      case ItemType::REF:
        if (!fully_reachable) {
          this->unresolved_refs_->emplace(((items::Ref*)item)->rule_name_);
        }
        break;
      default:
        break;
    }

    return fully_reachable;
  }

}
}
