#include <iostream>
#include <stdio.h>
#include <string>

#include "types.hpp"
#include "items/or.hpp"
#include "calc/reach.hpp"
#include "calc/ref_depth.hpp"
#include "rules.hpp"

namespace resmack {

  Rules::Rules(): Rules(NULL) {}
  Rules::Rules(Rules* parent): finalized_(false), parent_(parent) {}

  Rules::~Rules() {}

  Rules* Rules::AddRule(std::string name, std::string data) {
    this->rule_man_.Ensure(name)->AddItem(new items::Raw(data));
    return this;
  }

  Rules* Rules::AddRule(std::string name, Item* item) {
    this->rule_man_.Ensure(name)->AddItem(item);
    return this;
  }

  /**
   * Return true if the rule was successfully built
   */
  bool Rules::Build(std::string rule_name, std::string *output, Rand *rand) {
    return this->Build(rule_name, output, rand, 10);
  }

  bool Rules::Build(std::string rule_name, std::string *output, Rand *rand, size_t max_depth) {
    size_t rule_idx;
    if (!this->rule_man_.IndexOf(rule_name, &rule_idx)) {
      return false;
    }
    return this->Build(rule_idx, output, rand, max_depth);
  }

  /**
   * Return true if the rule was successfully built
   */
  bool Rules::Build(size_t rule_idx, std::string *output, Rand *rand) {
    return this->Build(rule_idx, output, rand, 10);
  }

  bool Rules::Build(size_t rule_idx, std::string *output, Rand *rand, size_t max_depth) {
    BuildContext ctx {
      .rules = NULL,
      .pre_output = NULL,
      .output = output,
      .rand = rand,
      .ref_depth = 0,
      .max_depth = max_depth,
    };
    return this->Build(rule_idx, &ctx);
  }

  bool Rules::Build(size_t rule_idx, BuildContext *ctx) {
    if (!this->finalized_) {
      this->Finalize();
    }

    if (ctx->rules == NULL) {
      ctx->rules = this;
    }

    std::string tmp_pre_output;
    if (ctx->pre_output == NULL) {
      ctx->pre_output = &tmp_pre_output;
    }

    items::Or* rule = this->rule_man_.GetRule(rule_idx);
    if (NULL == rule) { return false; }

    rule->Build(ctx);
    return true;
  }

  Rules* Rules::NewChild() {
    return new Rules(this);
  }

  void Rules::Finalize() {
    calc::Reach reach_calc(&this->rule_man_);
    calc::RefDepth ref_depth(&this->rule_man_);

    while (1) {
      reach_calc.Calc();
      ref_depth.Calc();
      if (reach_calc.NumChanges() + ref_depth.NumChanges() == 0) {
        break;
      }
    }

    this->finalized_ = true;
  }

}
