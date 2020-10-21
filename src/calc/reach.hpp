#ifndef RESMACK_CALC_REACH_H
#define RESMACK_CALC_REACH_H

#include <string>

#include "../types.hpp"
#include "../item.hpp"
#include "../items/or.hpp"
#include "../items/ref.hpp"

namespace resmack {
namespace calc {

  class Reach {
   public:
    Map<std::string, items::Or*>* map_;

    Set<std::string> unresolved_refs_;
    Set<std::string> pruned_;

    Set<std::string> tmp_new_rules_;
    Set<std::string> tmp_to_prune_;
    size_t num_changes_;

    Reach(Map<std::string, items::Or*>* map);
    ~Reach();

    void Calc();
    size_t NumChanges();
    bool CalcItem(Item* item);
    bool RuleExists(std::string rule_name);
  };

}
}

#endif
