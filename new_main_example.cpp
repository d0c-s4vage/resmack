#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>

#include "resmack/items/and.hpp"
#include "resmack/items/or.hpp"
#include "resmack/items/raw.hpp"
#include "resmack/items/ref.hpp"
#include "resmack/items/opt.hpp"
#include "resmack/rules.hpp"
#include "resmack/item.hpp"
#include "resmack/fuzz/cli/main.hpp"
#include "resmack/fuzz/interface.hpp"

static int count = 0;

size_t ResmackGrammarInit(resmack::Rules* rules) {
  rules->AddRule("fruit", OR("apples", "bananas", "grapes", "pears", "peaches"))
    ->AddRule("conjunction", OR("or", "and", "with", "without"))
    ->AddRule("fruit-list", AND(
      REF("fruit"),
      OPT(AND(V(" "), REF("conjunction"), V(" "), REF("fruit-list")))
    ))
    ->AddRule("verb", OR(
      "eat", "throw", "stomp on", "enjoy", "purchase", "stare at", "saute",
      "devour", "mock", "ridicule", "praise", "return", "investigate",
      "detest", "abhor", "congratulate"
    ))
    ->AddRule("subject", OR("I", "we", "you", "they"))
    ->AddRule("sentence", AND_S(" ", REF("subject"), REF("verb"), REF("fruit-list")))
    ->AddRule("run-on-sentence", AND(
      REF("sentence"),
      REF("run-on-sentence-opt")      
    ))
    ->AddRule("run-on-sentence-opt", OPT(AND(
      V(" "),
      REF("conjunction"),
      V(" "),
      REF("run-on-sentence")
    )))
    ->AddRule("direct", V("A"));

  size_t rule_idx;
  //if (!rules->GetRuleMan()->IndexOf("direct", &rule_idx)) {
  if (!rules->GetRuleMan()->IndexOf("run-on-sentence", &rule_idx)) {
    std::cout << "Invalid rules" << std::endl;
    std::exit(1);
  }

  return rule_idx;
}

void OtherFunction() {
  ((void(*)())(0))();
}

void NonAsanCrash() {
  OtherFunction();
}

void AsanCrash() {
  char data[10];
  const char* new_data = "THIS IS LONGER THAN THE ARRAY";
  memcpy(data, new_data, strlen(new_data));
}

extern "C"
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  //printf("FUZZING IT! pid: %d\n", getpid());
  //printf("Data: %.*s\n", size, data);
  //AsanCrash();
  //NonAsanCrash();
  return count++;
}

int main(int argc, char** argv) {
  resmack::fuzz::cli::Main(argc, argv);
}
