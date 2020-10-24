#include <ctime>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "rules.hpp"
#include "items/or.hpp"
#include "items/and.hpp"
#include "items/str.hpp"
#include "items/ref.hpp"
#include "items/opt.hpp"
#include "items/raw.hpp"
#include "items/int.hpp"
#include "items/pre_id.hpp"
#include "rand.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace resmack;

int main(int argc __attribute__((unused)), char** argv __attribute__((unused))) {
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
    ->AddRule("TestRule2", "---World");

  std::string output;
  output.reserve(0x1000);
  uint64_t total_bytes = 0;
  uint64_t count = 0;
  float start = clock() / (float)CLOCKS_PER_SEC;

  size_t rule_idx = 0;
  if (!rules.rule_man_.IndexOf("TestRule2", &rule_idx)) {
    return 0;
  }

  while (true) {
    count += 1;

    output.clear();
    rules.Build(rule_idx, &output, &rand);
    total_bytes += output.size();

    if (count % 0x800000 == 0) {
      float curr = clock() / (float)CLOCKS_PER_SEC;
      float totalMibs = (float)total_bytes / (1024.0f * 1024.0f);
      printf("%08lx | %0.2f iters/s | %0.2f MiB/s\n",
             count,
             (float)count / (curr - start),
             totalMibs / (curr - start));
    }
  }
}
