#include <iostream>
#include <string_view>
#include <vector>

#include "bazel_helpers.h"
#include <google/protobuf/util/json_util.h>


int main(int argc, char** argv) {
  std::vector<std::string_view> args;
  char** argPtr = argv;
  for (int ai = 0; ai < argc; ai++) {
    args.emplace_back(*argPtr);
    argPtr++;
  }

  const auto& [ec, qr] = BazelProjectManager::Internal::bazelQuery(
    ".",
    args.size() > 1 ? QString::fromUtf8(args[1].data(), args[1].length()) : QString{"//..."}
  );
  const auto n_targets = qr.target_size();

  std::string msgJson;
  google::protobuf::util::JsonPrintOptions jOps;
  jOps.add_whitespace = true;
  const auto jsonConvResult = google::protobuf::util::MessageToJsonString(qr, &msgJson, jOps);

  std::cout << msgJson << "\n";

  return 0;
}
