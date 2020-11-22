#include <cstring>

#include "resmack/build_context.hpp"
#include "resmack/rand.hpp"

namespace resmack {

  BuildContext::BuildContext(std::string* output, Rand* rand, size_t max_depth):
      rules(NULL),
      pre_output(NULL),
      output(output),
      post_output(NULL),
      rand(rand),
      ref_depth(0),
      max_depth(max_depth),
      replay(NULL),
      replay_idx(0),
      did_replay(false)
  {
  }

  void BuildContext::SetReplay(Vector<RandSnapshot>* replay) {
    this->replay = replay;
    this->replay_idx = 0;
  }

  void BuildContext::MaybeDoRandReplay(uint32_t tmp_state[]) {
    if (NULL == this->replay || this->replay_idx >= this->replay->size()) {
      this->did_replay = false;
      return;
    }

    RandSnapshot& snapshot = (*this->replay)[this->replay_idx];
    if (this->ref_depth != snapshot.ref_depth) {
      this->did_replay = false;
      return;
    }

    this->did_replay = true;
    this->rand->CopyState(tmp_state);
    this->rand->SetState(snapshot.state);
    this->replay_idx++;
  }

  void BuildContext::MaybeUndoRandReplay(uint32_t tmp_state[]) {
    if (!this->did_replay) {
      return;
    }

    this->rand->SetState(tmp_state);
  }

  bool BuildContext::DoShortest() {
    return this->ref_depth >= this->max_depth;
  }

  size_t BuildContext::IncDepth() {
    if (this->ref_depth == std::numeric_limits<size_t>::max()) {
      throw std::overflow_error("Attempted to increment ref depth past size_t max");
    }
    return this->ref_depth++;
  }
  size_t BuildContext::DecDepth() {
    if (this->ref_depth == 0) {
      throw std::overflow_error("Attempted to decrement ref depth past 0");
    }
    return this->ref_depth--;
  }

  void BuildContext::PrintDebugIo() {
    this->Message(std::string("CTX: ") + " pre_output: " + *this->pre_output);
    this->Message(std::string("CTX: ") + "     output: " + *this->output);
    this->Message(std::string("CTX: ") + "post_output: " + *this->post_output);
  }

  void BuildContext::Message(std::string msg) {
    std::string indent;
    for (size_t i = 0; i < this->ref_depth; i++) {
      indent += "  ";
    }
    indent += std::to_string(this->ref_depth) + "/" + std::to_string(this->max_depth);
    indent += "- ";

    size_t last_idx = 0;
    size_t newline_idx = msg.find("\n", last_idx);
    while (newline_idx != std::string::npos) {
      std::cout << indent << msg.substr(last_idx, newline_idx - last_idx) << std::endl;
    }
    std::cout << indent << msg.substr(last_idx, msg.size() - last_idx) << std::endl;
  }

}
