#ifndef RESMACK_ITEMS_FLUSH_H
#define RESMACK_ITEMS_FLUSH_H

#include "../item.hpp"

#define FLUSHED(VAL) (new resmack::items::Flushed(VAL))
#define FLUSH (new resmack::items::Flush())

namespace resmack {
namespace items {

  class Flush: public resmack::Item {
   public:
    ItemType Type();
    bool IntendsOutput() { return false; }
    void Build(BuildContext* ctx);
  };

  class Flushed: public resmack::Item {
   private:
    Item* item_;

   public:
    Flushed(Item* item);
    ~Flushed();
    ItemType Type();
    size_t CalcRefDepth(calc::RefDepth* calc);
    bool CalcReachability(calc::Reach* calc);
    bool IntendsOutput() { return true; }
    void Build(BuildContext* ctx);
  };

}
}

#endif
