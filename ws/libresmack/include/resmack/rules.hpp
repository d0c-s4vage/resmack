#ifndef RESMACK_RULES_H
#define RESMACK_RULES_H

#include "resmack/types.hpp"
#include "resmack/rule_man.hpp"
#include "resmack/rand.hpp"
#include "resmack/item.hpp"
#include "resmack/build_context.hpp"
#include "resmack/items/or.hpp"
#include "resmack/items/raw.hpp"

namespace resmack {

  class Rules {
   private:
    bool finalized_;

   public:
    Rules* parent_;
    RuleManager rule_man_;

    Rules();
    Rules(Rules* parent);
    ~Rules();

    Rules* GetParent() { return this->parent_; }
    Rules* AddRule(std::string name, Item* item);
    Rules* AddRule(std::string name, std::string data);
    Rules* AddRule(size_t rule_idx, Item* item);
    RuleManager* GetRuleMan() { return &this->rule_man_; }
    bool Build(std::string rule_name,
               std::string* output,
               Rand* rand);
    bool Build(std::string rule_name,
               std::string* output,
               Rand* rand,
               size_t max_depth);
    bool Build(size_t rule_idx,
               std::string* output,
               Rand* rand);
    bool Build(size_t rule_idx,
               std::string* output,
               Rand* rand,
               size_t max_depth);
    bool Build(size_t rule_idx, BuildContext* ctx);
    Rules* NewChild();
    void Finalize();
  };

}

#endif
