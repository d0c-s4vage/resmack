#include <ctime>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "resmack/defs.hpp"
#include "resmack/rules.hpp"
#include "resmack/items/or.hpp"
#include "resmack/items/and.hpp"
#include "resmack/items/str.hpp"
#include "resmack/items/ref.hpp"
#include "resmack/items/opt.hpp"
#include "resmack/items/raw.hpp"
#include "resmack/items/int.hpp"
#include "resmack/items/id.hpp"
#include "resmack/rand.hpp"

using namespace resmack;

int main(int argc, char** argv) {
  UNUSED(argc); UNUSED(argv);

  Rand rand;
  Rules rules;

  rules.AddRule("PruneMe", REF("unresolvable"))
    ->AddRule("PruneMeToo", REF("PruneMe"))
    ->AddRule("Special", "SPECIAL ONE")
    ->AddRule("RefdRule", OR("Hello", "Blah", "Special"))
    ->AddRule("RefdRule", OR(V("Hello"), V("Blah"), V("Special"), REF("TestRule")))
    ->AddRule("TestRule", AND(REF("RefdRule"), V("World")))
    ->AddRule("TestRule2", AND(REF("TestRule"), V("World")))
    ->AddRule("TestRule2", AND(REF("TestRule"), V("World")))
    ->AddRule("TestRule2", INT(5, 1337))
    ->AddRule("TestRule2", AND(OR(
      V("1"), V("2"), V("3"), V("4"), V("5"), STR(5, 10, "abcdefg")
    )))
    ->AddRule("TestRule2", "1000.5")
    ->AddRule("TestRule2", "---World")
    // run-on-sentence grammar
    ->AddRule("fruit", OR("apples", "bananas", "grapes", "pears", "peaches"))
    ->AddRule("conjunction", OR("or", "and", "with", "without"))
    ->AddRule("fruit-list", AND_S(" ",
      REF("fruit"),
      OPT(AND_S(" ", REF("conjunction"), REF("fruit-list")))
    ))
    ->AddRule("verb", OR(
      "eat", "throw", "stomp on", "enjoy", "purchase", "stare at", "saute",
      "devour", "mock", "ridicule", "praise", "return", "investigate",
      "detest", "abhor", "congratulate"
    ))
    ->AddRule("subject", OR("I", "we", "you"))
    ->AddRule("sentence", AND_S(" ", REF("subject"), REF("verb"), REF("fruit-list")))
    ->AddRule("run-on-sentence", AND(
      REF("sentence"),
      OPT(AND_S(" ", REF("conjunction"), REF("run-on-sentence")))
    ));

  std::string output;
  output.reserve(0x1000);
  uint64_t total_bytes = 0;
  uint64_t count = 0;
  float start = static_cast<float>(clock()) / (float)CLOCKS_PER_SEC;

  size_t rule_idx = 0;
  if (!rules.rule_man_.IndexOf("TestRule2", &rule_idx)) {
    return 0;
  }

  while (true) {
    count += 1;

    output.clear();
    rules.Build(rule_idx, &output, &rand, 10);
    total_bytes += output.size();

    if (count % 0x40000 == 0) {
      float curr = static_cast<float>(clock()) / (float)CLOCKS_PER_SEC;
      float totalMibs = (float)total_bytes / (1024.0f * 1024.0f);
      printf("%08lx | %0.2f iters/s | %0.2f MiB/s\n",
             count,
             (float)count / (curr - start),
             totalMibs / (curr - start));
    }
  }
}
