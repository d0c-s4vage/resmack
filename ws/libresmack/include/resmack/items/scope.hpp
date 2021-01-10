#ifndef RESMACK_SCOPE_H
#define RESMACK_SCOPE_H

#include "../item.hpp"

#define SCOPE(ITEM) (new resmack::items::Scope(ITEM))
#define SCOPE_PUSH (new resmack::items::ScopeAction(true))
#define SCOPE_POP (new resmack::items::ScopeAction(false))

namespace resmack {
namespace items {

  class Scope: public resmack::Item {
   public:
    Item* item_;

    Scope(Item* item);
    ~Scope();

    ItemType Type();
    void Build(BuildContext* ctx);
    bool IntendsOutput() { return false; }
    bool CalcReachability(calc::Reach* reach_calc);
    size_t CalcRefDepth(calc::RefDepth* ref_calc);
    std::string ToString();
  };

  class ScopeAction: public resmack::Item {
   private:
    bool push;

   public:
    ScopeAction(bool push);
    ~ScopeAction();

    ItemType Type();
    bool IntendsOutput() { return false; }
    void Build(BuildContext* ctx);
  };

}
}

#endif
