#include <random>

#include "resmack/item.hpp"
#include "resmack/items/opt.hpp"

namespace resmack {
namespace items {

  Opt::Opt(resmack::Item* item): item_(item) {}
  Opt::~Opt() { delete this->item_; }

  ItemType Opt::Type() {
   return TYPE_OPT;
  }

  void Opt::Build(BuildContext* ctx) {
    // % 3 seems so arbitrary here - Gaussian is slow at this level though
    //if (ctx->DoShortest() || (ctx->rand->Next() % (ctx->max_depth - ctx->ref_depth)) == 0) { return; }
    if (ctx->DoShortest() || ctx->rand->Maybe()) { return; }

    /*
    uint32_t tmp_val = ctx->rand->NextInRangeGaussian(
      ctx->ref_depth,
      // NORMAL DISTRIBUTION!
      //
      //                    ▄▄█▄▄
      //                   ███████
      //                  █████████
      //                 ▄█████████▄
      //                 ███████████
      //               ▄█████████████▄
      //            ▄▄█████████████████▄▄
      // 
      //                   ┌─ std dev
      //                ┌──┴──┐
      //          .     .     .     .     .
      // X────────│───────│───────│───────│──────────────X
      //          │               │
      //          │               └─── max_depth
      //          └─── ref_depth
      //
      ctx->max_depth + (ctx->max_depth - ctx->ref_depth) / 2
    );

    if (tmp_val >= ctx->max_depth) { return; }
    */

    this->item_->Build(ctx);
  }

  bool Opt::CalcReachability(calc::Reach* reach_calc) {
    // Always reachable, but we still need to calculate reachability
    // of the item
    this->item_->CalcReachability(reach_calc);
    return true;
  }

  size_t Opt::CalcRefDepth(calc::RefDepth *calc) {
    // minimum reference depth for this is 0, but we still need to calculate ref
    // depth of the item
    this->item_->CalcRefDepth(calc);
    return 0;
  }

}
}
