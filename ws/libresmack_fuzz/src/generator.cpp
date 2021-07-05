#include "resmack/build_context.hpp"
#include "resmack/fuzz/debug.hpp"
#include "resmack/fuzz/external.hpp"
#include "resmack/fuzz/generator.hpp"

namespace resmack {
namespace fuzz {

  Generator::Generator(
    size_t id,
    const GrammarConfig* config,
    ReplayInitCb replay_init_cb
  ) :
    max_depth(config->max_depth),
    replay_init_cb(replay_init_cb),
    id(id)
  {
    ExternalFunctions EF;
    this->start_rule_idx = EF.ResmackGrammarInit(&this->rules);
  }

  Generator::~Generator() {}

  std::string const* Generator::Generate() {
    BuildContext ctx(NULL, NULL, this->max_depth);

    if (!this->replay_init_cb(&this->base_replay)) {
      ctx.SetReplay(NULL);
    } else {
      ctx.SetReplay(&this->base_replay);
    }

    this->output.clear();
    this->rand.SnapshotClear();
    ctx.output = &this->output;
    ctx.rand = &this->rand;

    this->rules.Build(this->start_rule_idx, &ctx);

    return &this->output;
  }

}
}
