#include "resmack/build_context.hpp"
#include "resmack/fuzz/external.hpp"
#include "resmack/fuzz/generator.hpp"

namespace resmack {
namespace fuzz {

  GeneratedInfo::GeneratedInfo() : ready_cond(false), used_cond(false) {
  }

  // --------------------------------------------------------------------------

  Generator::Generator(const GrammarConfig* config, ReplayInitCb replay_init_cb) :
    to_generate_into(false), // 0
    to_consume_from(false), // 0
    max_depth(config->max_depth),
    replay_init_cb(replay_init_cb)
  {
    this->last_generated[0].used_cond.Signal();
    this->last_generated[1].used_cond.Signal();
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
      info->used_cond.Wait();

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

      info->ready_cond.Signal();
    }

    return NULL;
  }

  void Generator::NextInputWait() {
    this->last_generated[this->to_consume_from].ready_cond.Wait();
  }

  const std::string* Generator::NextInputGet() {
    return &this->last_generated[this->to_consume_from].output;
  }

  void Generator::NextInputUsed() {
    this->last_generated[this->to_consume_from].used_cond.Signal();
    this->to_consume_from = !this->to_consume_from;
  }
}
}
