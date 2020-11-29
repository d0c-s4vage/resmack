#ifndef RESMACK_ITEM_ID
#define RESMACK_ITEM_ID

#define ID(NAME) (new resmack::items::Id(NAME))

#include <string>

#include "../item.hpp"
#include "../build_context.hpp"
#include "../rules.hpp"

namespace resmack {
namespace items {

  class Id: public resmack::Item {
   public:
    std::string rule_name_;
    size_t rule_idx_;

    Id(std::string rule_name);
    ~Id();

    ItemType Type();
    void Build(BuildContext* ctx);
    bool CalcReachability(calc::Reach* reach_calc);
    size_t CalcRefDepth(calc::RefDepth* ref_calc);
    std::string ToString();
  };

}
}

#endif
