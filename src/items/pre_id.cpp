#include "../item.hpp"
#include "pre_id.hpp"
#include "raw.hpp"
#include "utils.hpp"
#include "str.hpp"

namespace resmack {
namespace items {

  PreId::PreId(std::string rule_name) : rule_name_(rule_name) {}
  PreId::~PreId() {}

  ItemType PreId::Type() {
    return ItemType::PRE_ID;
  }

  void PreId::Build(BuildContext *ctx) {
    std::string new_id;
    utils::RandBytes(ctx->rand, Str::CHARSET_ALPHA, 10, &new_id);
    ctx->rules->AddRule(this->rule_idx_, new Raw(new_id));
    *ctx->output += new_id;
  }

  bool PreId::CalcReachability(calc::Reach* reach_calc) {
    reach_calc->Ensure(this->rule_name_, &this->rule_idx_);
    return true;
  }

  size_t PreId::CalcRefDepth(__attribute__((unused)) calc::RefDepth* ref_calc) {
    return 0;
  }

  std::string PreId::ToString() {
    return std::string("<PreId ")
      + this->rule_name_
      + " (" + std::to_string(this->rule_idx_)
      + ")>";
  }

}
}
