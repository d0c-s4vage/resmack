#ifndef RESMACK_RULES_H
#define RESMACK_RULES_H

#include "types.hpp"
#include "rule_man.hpp"
#include "rand.hpp"
#include "item.hpp"
#include "calc/reach.hpp"
#include "calc/ref_depth.hpp"
#include "build_context.hpp"
#include "items/or.hpp"

namespace resmack {

  class Rules {
   private:
    bool finalized_;

   public:
    Rules* parent_;
    RuleManager rule_man_;

   public:
    Rules();
    Rules(Rules *parent);
    ~Rules();

    Rules* AddRule(std::string name, Item* item);
    RuleManager* GetRuleMan() { return &this->rule_man_; }
    bool Build(std::string rule_name,
               std::string *output,
               Rand *rand);
    bool Build(size_t rule_idx,
               std::string *output,
               Rand *rand);
    bool Build(std::string rule_name, BuildContext *ctx);
    bool Build(size_t rule_idx, BuildContext *ctx);
    Rules* NewChild();
    void Finalize();
  };

}

#endif
