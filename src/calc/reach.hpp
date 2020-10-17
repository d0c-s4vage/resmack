#ifndef RESMACK_CALC_REACH_H
#define RESMACK_CALC_REACH_H

#include <map>
#include <set>
#include <string>

#include "../item.hpp"
#include "../items/or.hpp"
#include "../items/ref.hpp"

namespace resmack {

namespace calc {

class Reach {
 public:
  std::map<std::string, items::Or*>* map_;

  std::set<std::string> unresolved_refs_;
  std::set<std::string> pruned_;

  std::set<std::string> tmp_new_rules_;
  std::set<std::string> tmp_to_prune_;
  size_t num_changes_;

  Reach(std::map<std::string, items::Or*>* map);
  ~Reach();

  void Calc();
  size_t NumChanges();
  bool CalcItem(Item* item);
  bool RuleExists(std::string rule_name);
};

}
}

#endif
