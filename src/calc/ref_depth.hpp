#ifndef RESMACK_CALC_REF_DEPTH_H
#define RESMACK_CALC_REF_DEPTH_H

#include <limits>
#include <string>

#include "types.hpp"
#include "item.hpp"
#include "items/or.hpp"

namespace resmack {
namespace calc {

  class RefDepth {
   private:
    Map<std::string, items::Or*>* map_;

    size_t num_changes_;
    Set<std::string> pruned_;
    Set<std::string> tmp_new_rules_;
    Set<std::string> tmp_to_prune_;
    Map<std::string, size_t> depths_;

   public:
    static const size_t INF_DEPTH = std::numeric_limits<size_t>::max();

    RefDepth(Map<std::string, items::Or*>* map);
    ~RefDepth();

    void Calc();
    size_t NumChanges() { return this->num_changes_; }
    size_t CalcItem(Item* item);
    size_t DepthOf(std::string rule_name);
  };

}
}

#endif
