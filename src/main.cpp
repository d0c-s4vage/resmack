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
#include "rand.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace resmack;

int main(int argc __attribute__((unused)), char** argv __attribute__((unused))) {
  Rand rand;
  Rules rules;

  rules.AddRule("PruneMe", new items::Ref("unresolvable"))
    ->AddRule("PruneMeToo", new items::Ref("PruneMe"))
    ->AddRule("Special", new items::Raw("SPECIAL ONE"))
    ->AddRule("RefdRule", (new items::Or())->AddItems(
      new items::Raw("Hello"),
      new items::Raw("Blah"),
      new items::Ref("Special"),
      NULL))
    ->AddRule("RefdRule", (new items::Or())->AddItems(
      new items::Raw("Hello"),
      new items::Raw("Blah"),
      new items::Ref("Special"),
      new items::Ref("TestRule"),
      NULL))
    ->AddRule("TestRule", (new items::And())->AddItems(
      new items::Ref("RefdRule"),
      new items::Raw("World"),
      NULL))
    ->AddRule("TestRule2", (new items::And())->AddItems(
      new items::Ref("TestRule"),
      new items::Raw("World"),
      NULL))
    ->AddRule("TestRule2", (new items::And())->AddItems(
      new items::Ref("TestRule"),
      new items::Raw("World"),
      NULL))
    ->AddRule("TestRule2", (new items::Int(5, 1337)))
    ->AddRule("TestRule2", (new items::And())->AddItem((new items::Or())->AddItems(
      new items::Raw("1"),
      new items::Raw("2"),
      new items::Raw("3"),
      new items::Raw("4"),
      new items::Raw("5"),
      new items::Str(5, 10, "abcdefg"),
      NULL)))
    ->AddRule("TestRule2", (new items::And())->AddItem(new items::Raw("1000.5")))
    ->AddRule("TestRule2", new items::Raw("---World"));

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
