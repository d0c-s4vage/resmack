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

  void Generator::InsertHooks(TargetHooks* hooks) {
    hooks->AddPrivateIpcSize([]() -> size_t {
      return sizeof(uint32_t) * 4;
    })->AddPrivateIpcInit([this](ipc::QueuedSharedMem* priv_mem) {
      this->last_rand_state = priv_mem->GetNextPtrFor<uint32_t>(sizeof(uint32_t) * 4);
    });
  }

  const Rand* Generator::GetRand() {
    return &this->rand;
  }

  std::string const* Generator::RegenerateLast() {
    uint32_t curr_state[4];
    this->rand.CopyState(curr_state);
    this->rand.SetState(this->last_rand_state);

    this->Generate();
    this->rand.SetState(&curr_state[0]);
    return &this->output;
  }

  std::string const* Generator::Generate() {
    // Maybe put this inside of a PreTest hook? makes more sense to me to
    // keep it here though...
    memcpy(this->last_rand_state, this->rand.GetState(), sizeof(uint32_t) * 4);

    BuildContext ctx(&this->output, &this->rand, this->max_depth);

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

  void Generator::ReinitRand(uint32_t seed) {
    // wrapping here is OK
    this->rand.InitState(this->rand.Next() + seed);
  }

}
}
