#ifndef RESMACK_CALC_REF_DEPTH_H
#define RESMACK_CALC_REF_DEPTH_H

#include <limits>
#include <map>
#include <set>
#include <string>

#include "item.hpp"
#include "items/or.hpp"

namespace resmack {
namespace calc {

  class RefDepth {
   private:
    std::map<std::string, items::Or*>* map_;

    size_t num_changes_;
    std::set<std::string> pruned_;
    std::set<std::string> tmp_new_rules_;
    std::set<std::string> tmp_to_prune_;
    std::map<std::string, size_t> depths_;

   public:
    static const size_t INF_DEPTH = std::numeric_limits<size_t>::max();

    RefDepth(std::map<std::string, items::Or*>* map);
    ~RefDepth();

    void Calc();
    size_t NumChanges() { return this->num_changes_; }
    size_t CalcItem(Item* item);
    size_t DepthOf(std::string rule_name);
  };

}
}

#endif
