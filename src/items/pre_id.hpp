#ifndef RESMACK_ITEM_PRE_ID
#define RESMACK_ITEM_PRE_ID

#define PRE_ID(NAME) (new resmack::items::PreId(NAME))

#include <string>

#include "../item.hpp"
#include "../build_context.hpp"
#include "../rules.hpp"

namespace resmack {
namespace items {

  class PreId: public resmack::Item {
   public:
    std::string rule_name_;
    size_t rule_idx_;

    PreId(std::string rule_name);
    ~PreId();

    ItemType Type();
    void Build(BuildContext *ctx);
    bool CalcReachability(calc::Reach* reach_calc);
    size_t CalcRefDepth(calc::RefDepth* ref_calc);
    std::string ToString();
  };

}
}

#endif
