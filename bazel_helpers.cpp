#include "bazel_helpers.h"

#include <cstdio>
#include <iostream>
#include <memory>

#include <fmt/format.h>

#include <3rd_party/bazel/src/main/protobuf/build.pb.h>


namespace BazelProjectManager::Internal {

blaze_query::QueryResult bazelQuery(const std::string_view query) {
  std::string cmd = fmt::format("bazel query {} --output proto", query);
  std::unique_ptr<FILE, decltype(&pclose)> stream{popen(cmd.c_str(), "r"), pclose};

  blaze_query::QueryResult qr;
  if (!qr.ParseFromFileDescriptor(fileno(stream.get()))) {
    throw std::runtime_error("Could not parse bazel output.");
  }
  return qr;
}

}  // namespace BazelProjectManager::Internal
