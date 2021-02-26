#include "resmack/build_context.hpp"
#include "resmack/fuzz/debug.hpp"
#include "resmack/fuzz/external.hpp"
#include "resmack/fuzz/generator.hpp"

namespace resmack {
namespace fuzz {

  GeneratedInfo::GeneratedInfo() : ready_cond(true), used_cond(true) {
  }

  // --------------------------------------------------------------------------

  Generator::Generator(
    size_t id,
    const GrammarConfig* config,
    ReplayInitCb replay_init_cb
  ) :
    to_generate_into(false), // 0
    to_consume_from(false), // 0
    max_depth(config->max_depth),
    replay_init_cb(replay_init_cb),
    id(id)
  {
    this->last_generated[0].used_cond.Signal();
    this->last_generated[0].ready_cond.Signal();
    this->last_generated[0].ready_cond.Wait();

    this->last_generated[1].used_cond.Signal();
    this->last_generated[1].ready_cond.Signal();
    this->last_generated[1].ready_cond.Wait();
  }

  Generator::~Generator() {}

  void Generator::Run() {
    this->should_run = true;
    ExternalFunctions EF;

    this->start_rule_idx = EF.ResmackGrammarInit(&this->rules);

    pthread_create(&this->generate_thread, NULL, &GenerateLoop, (void*)this);
  }

  void Generator::Stop() {
    this->should_run = false;
    this->last_generated[0].used_cond.Signal();
    this->last_generated[1].used_cond.Signal();

    pthread_join(this->generate_thread, NULL);
  }

  void* Generator::GenerateLoop(void* this_ptr) {
    Generator* this_ = reinterpret_cast<Generator*>(this_ptr);

    BuildContext ctx(NULL, NULL, this_->max_depth);

    while (this_->should_run) {
      GeneratedInfo* info = &this_->last_generated[this_->to_generate_into];
      _DEBUG_PRINT("%lu: Generator: Waiting for used_cond\n", this_->id);
      info->used_cond.Wait();
      _DEBUG_PRINT("%lu: Generator: Got the used_cond\n", this_->id);

      if (!this_->replay_init_cb(&this_->base_replay)) {
        ctx.SetReplay(NULL);
      } else {
        ctx.SetReplay(&this_->base_replay);
      }

      info->output.clear();
      info->rand.SnapshotClear();
      ctx.output = &info->output;
      ctx.rand = &info->rand;

      this_->rules.Build(this_->start_rule_idx, &ctx);
      this_->to_generate_into = !this_->to_generate_into;

      _DEBUG_PRINT("%lu: Generator: Signalling ready cond\n", this_->id);
      info->ready_cond.Signal();
    }

    return NULL;
  }

  void Generator::NextInputWait() {
    _DEBUG_PRINT("%lu: >>Generator: Waiting for ready cond\n", this->id);
    this->last_generated[this->to_consume_from].ready_cond.Wait();
    _DEBUG_PRINT("%lu: >>Generator: Waiting for ready cond - ready!\n", this->id);
  }

  const std::string* Generator::NextInputGet() {
    return &this->last_generated[this->to_consume_from].output;
  }

  void Generator::NextInputUsed() {
    _DEBUG_PRINT("%lu: >>Generator: Signalling used cond\n", this->id);
    bool to_signal = this->to_consume_from;
    this->to_consume_from = !this->to_consume_from;
    this->last_generated[to_signal].used_cond.Signal();
    _DEBUG_PRINT("%lu: >>Generator: Signalling used cond - signalled!\n", this->id);
  }
}
}
