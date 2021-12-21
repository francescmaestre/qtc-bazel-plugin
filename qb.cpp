#include <iostream>

#include "bazel_helpers.h"
#include <google/protobuf/util/json_util.h>


int main(int argc, char** argv) {
  const auto& [ec, qr] = BazelProjectManager::Internal::bazelQuery(".", "//...");
  const auto n_targets = qr.target_size();
  std::cout << "Bazel exited with " << ec << ". Got " << n_targets << " targets:\n";

  std::string msgJson;
  google::protobuf::util::JsonPrintOptions jOps;
  jOps.add_whitespace = true;
  const auto jsonConvResult = google::protobuf::util::MessageToJsonString(qr, &msgJson, jOps);

  std::cout << msgJson << "\n";

  return 0;
}
