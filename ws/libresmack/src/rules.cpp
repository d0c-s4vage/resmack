#include <cstring>
#include <functional>
#include <memory>
#include <stdio.h>
#include <string>
#include <unistd.h>

#include "resmack/items/or.hpp"
#include "resmack/rules.hpp"
#include "resmack/debug.hpp"

#include "calc/reach.hpp"
#include "calc/ref_depth.hpp"

using namespace std;
using defer = shared_ptr<void>;

namespace resmack {

  Rules::Rules() : Rules(NULL) {}

  Rules::Rules(Rules* parent): finalized_(false) {
    this->parent_ = parent;
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
    this->rule_man_.Ensure(rule_idx)->AddItem(item);
    return this;
  }

  /**
   * Return true if the rule was successfully built
   */
  bool Rules::Build(std::string rule_name, std::string* output, Rand* rand) {
    return this->Build(rule_name, output, rand, 10);
  }

  bool Rules::Build(std::string rule_name, std::string* output, Rand* rand, size_t max_depth) {
    size_t rule_idx;
    if (!this->rule_man_.IndexOf(rule_name, &rule_idx)) {
      return false;
    }
    return this->Build(rule_idx, output, rand, max_depth);
  }

  bool Rules::Build(size_t rule_idx, std::string* output, Rand* rand) {
    return this->Build(rule_idx, output, rand, 10);
  }

  bool Rules::Build(size_t rule_idx, std::string* output, Rand* rand, size_t max_depth) {
    BuildContext ctx(output, rand, max_depth);
    return this->Build(rule_idx, &ctx);
  }

  bool Rules::Build(size_t rule_idx, BuildContext* ctx) {
    return Build(rule_idx, ctx, false);
  }

  // Do not look into parent scopes when building, only the local scope
  bool Rules::Build(size_t rule_idx, BuildContext* ctx, bool unshadowed) {
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

    items::Or* rule;
    if (unshadowed) {
      rule = this->rule_man_.GetUnshadowedRule(rule_idx);
    } else {
      rule = this->rule_man_.GetAnyRule(rule_idx, ctx->rand);
    }

    if (NULL == rule) { return false; }

    uint32_t tmp_rand_state[4] = { 0, 0, 0, 0 };
    uint32_t tmp_max_depth = ctx->max_depth;
    bool tmp_did_replay = false;

    ctx->MaybeDoRandReplay(tmp_rand_state, &tmp_max_depth, &tmp_did_replay);
    if (ctx->rand->ShouldRecord()) {
      ctx->rand->SnapshotState(ctx->ref_depth, ctx->max_depth, rule_idx);
    }
    rule->Build(ctx);
    ctx->MaybeUndoRandReplay(tmp_rand_state, tmp_max_depth, tmp_did_replay);

    // flush all pre/post output
    if (ctx->ref_depth == 0) {
      ctx->FlushPrePost();
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
