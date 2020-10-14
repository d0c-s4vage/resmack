#include <iostream>
#include <map>
#include <stdio.h>

#include "rules.hpp"
#include "items/or.hpp"

namespace resmack {

Rules::Rules() {
  std::cout << "Constructed!\n";
}

Rules::~Rules() {
  for (auto it = this->map_.begin(); it != this->map_.end(); it++) {
    std::string key = it->first;
    items::Or* val = it->second;
    delete val;
  }
}

Rules* Rules::AddRule(std::string name, Item* item) {
  if (!this->map_.contains(name)) {
    this->map_.emplace(name, new items::Or());
  }
  this->map_[name]->AddItem(item);

  return this;
}

/**
 * Return true if the rule was successfully built
 */
bool Rules::Build(std::string rule_name, std::string *output, Rand *rand) {
  BuildContext ctx {
    .rules = NULL,
    .pre_output = NULL,
    .output = output,
    .rand = rand,
  };
  return this->Build(rule_name, &ctx);
}

bool Rules::Build(std::string rule_name, BuildContext* ctx) {
  std::string tmp_pre_output;
  if (ctx->rules == NULL) {
    ctx->rules = this;
  }
  if (ctx->pre_output == NULL) {
    ctx->pre_output = &tmp_pre_output;
  }
  if (!this->map_.contains(rule_name)) {
    return false;
  }
  this->map_[rule_name]->Build(ctx);

  return true;
}

}
