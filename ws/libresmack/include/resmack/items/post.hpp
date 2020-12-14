#ifndef RESMACK_POST
#define RESMACK_POST

#define POST(ITEM) (new resmack::items::Post(ITEM))

#include <string>

#include "../item.hpp"
#include "../build_context.hpp"
#include "../rules.hpp"

namespace resmack {
namespace items {

  class Post: public resmack::Item {
   private:
    Item* item_;

   public:
    Post(Item* item);
    ~Post();

    ItemType Type();
    void Build(BuildContext* ctx);
    bool IntendsOutput() { return false; }
    bool CalcReachability(calc::Reach* reach_calc);
    size_t CalcRefDepth(calc::RefDepth* ref_calc);
    std::string ToString();
  };

}
}

#endif
