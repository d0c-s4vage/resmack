#ifndef RESMACK_RULES_H
#define RESMACK_RULES_H

#include "types.hpp"
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
    Map<std::string, items::Or*> map_;

   public:
    Rules();
    Rules(Rules *parent);
    ~Rules();

    Map<std::string, items::Or*>* GetRules() { return &this->map_; }
    Rules* AddRule(std::string name, Item* item);
    bool Build(std::string rule_name,
               std::string *output,
               Rand *rand);
    bool Build(std::string rule_name, BuildContext *ctx);
    Rules* NewChild();
    void Finalize();
  };

}

#endif
