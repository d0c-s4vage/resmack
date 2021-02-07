#include "resmack/items/capture.hpp"

namespace resmack {
namespace items {
  // one per ref level, can be overridden. The ref levels act as our stack
  // of CAPTURES (which is definitely possible)
  // consider:
  //
  //   Obj1 := AND(
  //     PRE(AND(
  //        V("var "), CAPTURE(STR(5, 6)), V(" = new Object()")
  //     )),
  //     CAPTURED
  //   )
  //   Obj2 := 
  //     PRE(AND(
  //        V("var "), CAPTURE(STR(5, 6)), V(" = "), REF("Obj1")
  //     )),
  //     CAPTURED
  //   )
  //
  // The build context has a Vector<std::string> captures field

  // --------------------------------------------------------------------------
  // CAPTURE ------------------------------------------------------------------
  // --------------------------------------------------------------------------

  Capture::Capture(Item* item) : item_(item) {}
  Capture::~Capture() { delete this->item_; }

  ItemType Capture::Type() { return TYPE_CAPTURE; }

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

    while (ctx->captures.size() <= ctx->ref_depth) {
      ctx->captures.emplace_back();
    }
    ctx->captures[ctx->ref_depth].assign(ctx->output->data() + start_pos, len);
  }

  // --------------------------------------------------------------------------
  // CAPTURED -----------------------------------------------------------------
  // --------------------------------------------------------------------------

  ItemType Captured::Type() {
    return TYPE_CAPTURED;
  }

  void Captured::Build(BuildContext* ctx) {
    *(ctx->output) += ctx->captures[ctx->ref_depth];
  }
}
}
