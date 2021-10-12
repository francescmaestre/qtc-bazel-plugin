#include <iostream>

#include "bazel_helpers.h"
#include <3rd_party/bazel/src/main/protobuf/build.pb.h>


int main(int argc, char** argv) {
  const auto& qr = BazelProjectManager::Internal::bazelQuery("//...");
  const auto n_targets = qr.target_size();
  std::cout << "Got " << n_targets << " targets:\n";
  for (int i = 0; i < n_targets; i++) {
    const auto& target = qr.target(i);
    std::cout << " - " << blaze_query::Target::Discriminator_Name(target.type()) << "\n";
  }

  return 0;
}
