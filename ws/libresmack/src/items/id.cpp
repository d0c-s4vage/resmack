#include "resmack/defs.hpp"
#include "resmack/item.hpp"
#include "resmack/items/id.hpp"
#include "resmack/items/raw.hpp"
#include "resmack/items/str.hpp"

#include "calc/reach.hpp"
#include "calc/ref_depth.hpp"
#include "utils.hpp"

namespace resmack {
namespace items {

  Id::Id(std::string rule_name) : rule_name_(rule_name) {}
  Id::~Id() {}

  ItemType Id::Type() {
    return ItemType::ID;
  }

  void Id::Build(BuildContext *ctx) {
    std::string new_id;
    utils::RandBytes(ctx->rand, CHARSET_ALPHA, sizeof(CHARSET_ALPHA), 10, &new_id);
    ctx->rules->AddRule(this->rule_idx_, new Raw(new_id));
    *ctx->output += new_id;
  }

  bool Id::CalcReachability(calc::Reach* reach_calc) {
    reach_calc->Ensure(this->rule_name_, &this->rule_idx_);
    return true;
  }

  size_t Id::CalcRefDepth(calc::RefDepth* ref_calc) {
    UNUSED(ref_calc);
    return 0;
  }

  std::string Id::ToString() {
    return std::string("<Id ")
      + this->rule_name_
      + " (" + std::to_string(this->rule_idx_)
      + ")>";
  }

}
}
