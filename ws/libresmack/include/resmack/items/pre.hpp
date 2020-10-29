#ifndef RESMACK_PRE
#define RESMACK_PRE

#define PRE(ITEM) (new resmack::items::Pre(ITEM))

#include <string>

#include "../item.hpp"
#include "../build_context.hpp"
#include "../rules.hpp"

namespace resmack {
namespace items {

  class Pre: public resmack::Item {
   private:
    Item* item_;

   public:
    Pre(Item* item);
    ~Pre();

    ItemType Type();
    void Build(BuildContext *ctx);
    bool CalcReachability(calc::Reach* reach_calc);
    size_t CalcRefDepth(calc::RefDepth* ref_calc);
    std::string ToString();
  };

}
}

#endif
