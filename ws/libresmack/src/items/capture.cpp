#include "resmack/items/capture.hpp"

namespace resmack {
namespace items {

  // --------------------------------------------------------------------------
  // CAPTURE ------------------------------------------------------------------
  // --------------------------------------------------------------------------

  Capture::Capture(Item* item) : item_(item) {}
  Capture::~Capture() { delete this->item_; }

  ItemType Capture::Type() { return ItemType::CAPTURE; }

  bool Capture::CalcReachability(calc::Reach* calc) {
    return this->item_->CalcReachability(calc);
  }

  size_t Capture::CalcRefDepth(calc::RefDepth* calc) {
    return this->item_->CalcRefDepth(calc);
  }

  void Capture::Build(BuildContext* ctx) {
    size_t start_pos = ctx->output->size();
    this->item_->Build(ctx);
    size_t len = ctx->output->size() - start_pos;

    CAPTURED_DATA.assign(ctx->output->data() + start_pos, len);
  }

  // --------------------------------------------------------------------------
  // CAPTURED -----------------------------------------------------------------
  // --------------------------------------------------------------------------

  ItemType Captured::Type() {
    return ItemType::CAPTURED;
  }

  void Build(BuildContext* ctx) {
    *(ctx->output) += CAPTURED_DATA;
  }
}
}
