#include "resmack/item.hpp"
#include "resmack/items/opt.hpp"

namespace resmack {
namespace items {

  Opt::Opt(resmack::Item* item): item_(item) {}
  Opt::~Opt() { delete this->item_; }

  ItemType Opt::Type() {
   return ItemType::OPT;
  }

  void Opt::Build(BuildContext* ctx) {
   if (ctx->rand->Maybe()) {
     this->item_->Build(ctx);
   }
  }

}
}
