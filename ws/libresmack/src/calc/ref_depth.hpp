#ifndef RESMACK_CALC_REF_DEPTH_H
#define RESMACK_CALC_REF_DEPTH_H

#include <limits>
#include <string>

#include "resmack/rule_man.hpp"
#include "resmack/types.hpp"
#include "resmack/item.hpp"
#include "resmack/items/or.hpp"

namespace resmack {
namespace calc {
  class RefDepth {
   private:
    RuleManager* rule_man_;

    size_t num_changes_;
    Set<size_t> pruned_;
    Set<std::string> tmp_new_rules_;
    Set<size_t> tmp_to_prune_;
    Map<size_t /*rule_idx*/, size_t /*depth*/> depths_;

   public:
    inline static constexpr size_t INF_DEPTH = std::numeric_limits<size_t>::max();

    RefDepth(RuleManager* rule_man);
    ~RefDepth();

    void Calc();
    size_t NumChanges() { return this->num_changes_; }
    size_t CalcItem(Item* item);
    size_t DepthOf(std::string rule_name);
  };

}
}

#endif
