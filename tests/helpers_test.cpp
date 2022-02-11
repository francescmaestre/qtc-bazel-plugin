#include <catch2/catch.hpp>

#include <bazel_helpers.h>

namespace BazelProjectManager::Internal {

TEST_CASE() {
  BazelPackage root{};

  REQUIRE(QString{"/"}.startsWith(""));

  REQUIRE(root.dirPath() == "/");
  REQUIRE(root.bazelPath() == "//");
  REQUIRE_FALSE(root.isConsumedBy(QString{"/..."}));
  REQUIRE_FALSE(root.isConsumedBy(QString{"//..."}));

  root.subPackages.push_back(
    std::make_shared<BazelPackage>(
      "p1",
      &root,
      BazelPackage::ChildrenContainerType{},
      BazelPackage::TargetsContainerType{}
    )
  );

  auto& p1 = root.subPackages.front();

  REQUIRE(p1->dirPath() == "/p1");
  REQUIRE(p1->bazelPath() == "//p1");

  REQUIRE(p1->isConsumedBy(QString{"//..."}));
  REQUIRE_FALSE(p1->isConsumedBy(QString{"//p1/..."}));
}

}  // namespace BazelProjectManager::Internal
