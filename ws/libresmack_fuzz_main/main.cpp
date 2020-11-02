#include <string>

int main(int argc, char** argv) {
  Rules rules = new Rules();
  LoadRules(&rules);
  std::string rule_name;
  size_t start_idx = rules.GetRuleIdx(rule_name);

  resmack::fuzz::Coverage cov;
  resmack::fuzz::Corpus corpus;
  resmack::Rand rand;
  resmack::fuzz::TestResult test_result;

  Vector<resmack::RandSnapshot> curr_rand_tree;

  rand.SetRecord(true);

  std::string output;
  output.reserve(0x1000);

  while (true) {
    output.clear();
    cov.Clear();
    rand.ClearDecisions();
    curr_rand_tree.clear();

    if (corpus.Size() > 0 && rand.Maybe()) {
      Vector<resmack::RandSnapshot>* corpus_tree = corpus.FetchRandSnapshotTree();
      resmack::fuzz::MutateRandSnapshot(&rand, corpus_tree, &curr_rand_tree);
      rules.SetRandSnapshotTree(&curr_rand_tree);
    }

    rules.Build(start_idx, &output, &rand);
    DoResmackTestOneInput(output, &test_result);

    corpus.HandleTestResults(&test_result, &curr_rand_tree);
  }
}
