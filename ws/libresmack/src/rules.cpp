#include <iostream>
#include <stdio.h>
#include <string>
#include <cstring>

#include "resmack/types.hpp"
#include "resmack/items/or.hpp"
#include "resmack/rules.hpp"

#include "calc/reach.hpp"
#include "calc/ref_depth.hpp"

namespace resmack {

  Rules::Rules(): Rules(NULL) {}
  Rules::Rules(Rules* parent): finalized_(false), parent_(parent) {
    if (parent != NULL) {
      this->rule_man_.SetParent(parent->GetRuleMan());
    } else {
      this->rule_man_.Init();
    }
  }

  Rules::~Rules() {}

  Rules* Rules::AddRule(std::string name, std::string data) {
    this->rule_man_.Ensure(name)->AddItem(new items::Raw(data));
    return this;
  }

  Rules* Rules::AddRule(std::string name, Item* item) {
    this->rule_man_.Ensure(name)->AddItem(item);
    return this;
  }

  Rules* Rules::AddRule(size_t rule_idx, Item* item) {
    this->rule_man_.GetRule(rule_idx)->AddItem(item);
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

  bool Rules::Build(size_t rule_idx, std::string *output, Rand *rand) {
    return this->Build(rule_idx, output, rand, 10);
  }

  bool Rules::Build(size_t rule_idx, std::string *output, Rand *rand, size_t max_depth) {
    BuildContext ctx(output, rand, max_depth);
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

    std::string tmp_post_output;
    if (ctx->post_output == NULL) {
      ctx->post_output = &tmp_post_output;
    }

    items::Or* rule = this->rule_man_.GetAnyRule(rule_idx, ctx->rand);
    if (NULL == rule) { return false; }

    uint32_t tmp_rand_state[4] = { 0, 0, 0, 0 };
    ctx->MaybeDoRandReplay(tmp_rand_state);
    {
      if (ctx->rand->ShouldRecord()) {
        ctx->rand->SnapshotState(ctx->ref_depth);
      }

      rule->Build(ctx);
    }
    ctx->MaybeUndoRandReplay(tmp_rand_state);

    if (ctx->ref_depth == 0) {
      if (ctx->pre_output->size() > 0) {
        (*ctx->output) = *ctx->pre_output + *ctx->output;
        ctx->pre_output->clear();
      }
      if (ctx->post_output->size() > 0) {
        (*ctx->output) += *ctx->post_output;
        ctx->post_output->clear();
      }
    }

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
