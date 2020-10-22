#include "rule_man.hpp"
#include "items/or.hpp"

namespace resmack {

  RuleManager::RuleManager() {}
  RuleManager::~RuleManager() {
    for (size_t idx = 0; idx < this->rules_.size(); idx++) {
      delete this->rules_[idx];
      // keep the rule->idx and idx->rule mappings, this way we'll know
      // that it existed at one point, but was pruned
      this->rules_[idx] = NULL;
    }
  }

  Vector<items::Or*>* RuleManager::GetRules() {
    return &this->rules_;
  }

  // A null value for `out` is allowed
  bool RuleManager::IndexOf(std::string rule_name, size_t* out) {
    if (this->rule_name_to_idx_.contains(rule_name)) {
      if (out != NULL) {
        *out = this->rule_name_to_idx_[rule_name];
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
    if (this->rule_idx_to_name_.contains(rule_idx)) {
      if (out != NULL) {
        *out = this->rule_idx_to_name_[rule_idx];
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

    if (this->rule_name_to_idx_.contains(rule_name)) {
      rule_idx = this->rule_name_to_idx_[rule_name];
      res = this->rules_[rule_idx];
    } else {
      rule_idx = this->rules_.size();
      this->rule_name_to_idx_[rule_name] = rule_idx;
      this->rule_idx_to_name_[rule_idx] = rule_name;
      res = new items::Or();
      this->rules_.emplace_back(res);
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

  items::Or* RuleManager::GetRule(size_t rule_idx) {
    if (rule_idx >= this->rules_.size()) { return NULL; }
    return this->rules_[rule_idx];
  }
}
