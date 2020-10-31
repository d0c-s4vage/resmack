#ifndef RESMACK_CALC_REACH_H
#define RESMACK_CALC_REACH_H

#include <string>

#include "resmack/item.hpp"
#include "resmack/items/or.hpp"
#include "resmack/items/ref.hpp"
#include "resmack/rule_man.hpp"
#include "resmack/types.hpp"

namespace resmack {
namespace calc {

  class Reach {
   public:
    RuleManager* rule_man_;

    Set<std::string> unresolved_refs_;
    Set<size_t> pruned_;

    Set<std::string> tmp_new_rules_;
    Set<size_t> tmp_to_prune_;
    size_t num_changes_;

    Reach(RuleManager* rule_man);
    ~Reach();

    void Calc();
    size_t NumChanges();
    bool CalcItem(Item* item);
    bool IndexOf(std::string rule_name, size_t* out);
    void Ensure(std::string rule_name, size_t* out);
  };

}
}

#endif
