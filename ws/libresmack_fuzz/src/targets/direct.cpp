// #include <string>
// #include <iostream>
// 
// #include "resmack/fuzz/targets/direct.hpp"
// #include "resmack/fuzz/target.hpp"
// #include "resmack/fuzz/external.hpp"
// 
// /*
// extern int ResmackTestOneInput(const unsigned char *data, size_t size);
// extern int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size);
// */
// 
// namespace resmack {
// namespace fuzz {
// 
//   static ExternalFunctions EF;
// 
//   DirectTarget::DirectTarget() {
//   }
// 
//   void DirectTarget::Launch(
//     Feedback* feedback,
//     std::string* output,
//     TargetSettings* settings,
//     TargetStats* stats
//   ) {
//     UNUSED(settings);
// 
//     stats->Reset();
// 
//     RECORD_STAT(stats, SampleTypes::FEEDBACK, {
//       feedback->Start();
//     });
// 
//     stats->valid = false;
//     RECORD_STAT(stats, SampleTypes::TARGET, {
//       stats->valid = EF.LLVMFuzzerTestOneInput((const uint8_t*)output->data(), output->size());
//     });
// 
//     RECORD_STAT(stats, SampleTypes::FEEDBACK, {
//       feedback->Stop();
//     });
//   }
// 
//   void DirectTarget::Reset() {}
// 
// }
// }
