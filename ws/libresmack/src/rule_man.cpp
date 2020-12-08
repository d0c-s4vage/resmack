#include "resmack/rule_man.hpp"
#include "resmack/items/or.hpp"

namespace resmack {

  RuleManager::RuleManager(): parent_(NULL) {}
  RuleManager::~RuleManager() {
    for (size_t idx = 0; idx < this->rules_.size(); idx++) {
      delete this->rules_[idx];
      // keep the rule->idx and idx->rule mappings, this way we'll know
      // that it existed at one point, but was pruned
      this->rules_[idx] = NULL;
    }

    if (this->parent_ == NULL) {
      delete this->rule_idx_to_name_;
      delete this->rule_name_to_idx_;
    }
  }

  void RuleManager::Init() {
    this->rule_idx_to_name_ = new Map<size_t, std::string>();
    this->rule_name_to_idx_ = new Map<std::string, size_t>();
  }

  void RuleManager::SetParent(RuleManager* parent) {
    this->parent_ = parent;
    this->rule_idx_to_name_ = parent->rule_idx_to_name_;
    this->rule_name_to_idx_ = parent->rule_name_to_idx_;

    size_t num_rules = parent->NumRules();
    this->rules_.reserve(num_rules);
    for (size_t i = 0; i < num_rules; i++) {
      this->rules_.push_back((items::Or*)NULL);
    }
  }

  Vector<items::Or*>* RuleManager::GetRules() {
    return &this->rules_;
  }

  // A null value for `out` is allowed
  bool RuleManager::IndexOf(std::string rule_name, size_t* out) {
    if (this->rule_name_to_idx_->contains(rule_name)) {
      if (out != NULL) {
        *out = (*this->rule_name_to_idx_)[rule_name];
      }
      return true;
    }
    return false;
  }

  bool RuleManager::IndexExists(size_t rule_idx) {
    return this->NameOf(rule_idx, NULL);
  }

  // A null value for `out` is allowed
  bool RuleManager::NameOf(size_t rule_idx, std::string* out) {
    if (this->rule_idx_to_name_->contains(rule_idx)) {
      if (out != NULL) {
        *out = (*this->rule_idx_to_name_)[rule_idx];
      }
      return true;
    }
    return false;
  }

  bool RuleManager::NameExists(std::string rule_name) {
    return this->IndexOf(rule_name, NULL);
  }

  bool RuleManager::ValidRule(size_t rule_idx) {
    return this->IndexExists(rule_idx) && this->rules_[rule_idx] != NULL;
  }

  bool RuleManager::ValidRule(std::string rule_name) {
    size_t rule_idx;
    if (!this->IndexOf(rule_name, &rule_idx)) {
      return false;
    }
    return this->ValidRule(rule_idx);
  }

  items::Or* RuleManager::Ensure(std::string rule_name) {
    items::Or* res;
    size_t rule_idx;

    if (this->IndexOf(rule_name, &rule_idx)) {
      res = this->rules_[rule_idx];
      if (res == NULL) {
        res = new items::Or();
        this->rules_[rule_idx] = res;
      }
    } else {
      rule_idx = this->rules_.size();
      (*this->rule_name_to_idx_)[rule_name] = rule_idx;
      (*this->rule_idx_to_name_)[rule_idx] = rule_name;
      res = new items::Or();
      this->rules_.push_back(res);
    }

    return res;
  }

  items::Or* RuleManager::Ensure(size_t rule_idx) {
    items::Or* res = this->GetRule(rule_idx);
    if (res == NULL) {
      res = new items::Or();
      this->rules_[rule_idx] = res;
    }
    return res;
  }

  void RuleManager::Prune(std::string rule_name) {
    size_t rule_idx;
    if (!this->IndexOf(rule_name, &rule_idx)) { return; }

    this->Prune(rule_idx);
  }

  void RuleManager::Prune(size_t rule_idx) {
    if (rule_idx >= this->rules_.size()) { return; }

    delete this->rules_[rule_idx];
    this->rules_[rule_idx] = 0;
  }

  items::Or* RuleManager::GetRule(std::string rule_name) {
    size_t rule_idx;
    if (!this->IndexOf(rule_name, &rule_idx)) { return NULL; }

    return this->GetRule(rule_idx);
  }

  items::Or* RuleManager::GetAnyRule(size_t rule_idx, Rand* rand) {
    if (this->parent_ == NULL) {
      return this->GetRule(rule_idx);
    }
    Vector<RuleManager*> options;
    RuleManager* curr = this;
    while (curr != NULL) {
      if (curr->ValidRule(rule_idx)) {
        options.push_back(curr);
      }
      curr = curr->parent_;
    }

    size_t idx = rand->Next() % options.size();
    return options[idx]->GetRule(rule_idx);
  }

  items::Or* RuleManager::GetRule(size_t rule_idx) {
    if (rule_idx >= this->rules_.size()) {
      return NULL;
    }
    return this->rules_[rule_idx];
  }

  void RuleManager::DebugPrint() {
    std::cout << "Rules:" << std::endl;
    for (size_t idx = 0; idx < this->rules_.size(); idx++) {
      std::cout << "  " << idx << ": " << (*this->rule_idx_to_name_)[idx] << ": ";
      items::Or* rule = this->rules_[idx];
      if (rule == NULL) {
        std::cout << "NULL" << std::endl;
      } else {
        std::cout << rule->ToString() << std::endl;
      }
    }
  }
}
