#ifndef RESMACK_ITEM_REF
#define RESMACK_ITEM_REF

#define REF(NAME) (new resmack::items::Ref(NAME))

#include <string>

#include "../item.hpp"
#include "../build_context.hpp"

namespace resmack {
namespace items {

  class Ref: public resmack::Item {
   public:
    std::string rule_name_;
    size_t rule_idx_;

    Ref(std::string rule_name);
    ~Ref();

    ItemType Type();
    void Build(BuildContext *ctx);
    bool CalcReachability(calc::Reach* reach_calc);
    size_t CalcRefDepth(calc::RefDepth* ref_calc);
    std::string ToString();
  };

}
}

#endif
