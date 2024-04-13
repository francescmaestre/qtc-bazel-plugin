#include <catch2/catch.hpp>

#include <BazelWorkspace.h>

namespace Catch {
template <>
struct StringMaker<QString> {
  static std::string convert(QString const& value) { return "\"" + value.toStdString() + "\""; }
};
}  // namespace Catch

namespace BazelProjectManager::Internal::tests {

TEST_CASE() {
  BazelWorkspace workspace{Utils::FilePath::fromString("/workspace/dir")};

  auto root = workspace.rootPackage();
  REQUIRE(root);
  CHECK(root->dirPath() == QString{"/"});
  CHECK(root->bazelPath() == "//");
  CHECK_FALSE(root->isConsumedBy(QString{"/..."}));
  CHECK_FALSE(root->isConsumedBy(QString{"//..."}));

  {
    auto p1 = root->addSubDirectories(QString{"/some/p1"});
    auto p2 = root->addSubDirectories(QString{"/some/p2"});

    REQUIRE(p1);
    CHECK(p1->name() == "p1");
    CHECK(p1->dirPath() == QString{"/some/p1"});

    REQUIRE(p2);
    CHECK(p2->name() == "p2");
    CHECK(p2->dirPath() == QString{"/some/p2"});

    auto p3 = p2->addSubDirectories(QString{"/p3"});
    REQUIRE(p3);
    CHECK(p3->name() == "p3");
    CHECK(p3->dirPath() == QString{"/some/p2/p3"});
  }

  {
    auto p1 = root->addSubDirectories(QString{"/some/path/sub-package1"});
    REQUIRE(p1);

    CHECK(p1->name() == "sub-package1");
    CHECK(p1->dirPath() == QString{"/some/path/sub-package1"});
    CHECK(p1->bazelPath() == "//some/path/sub-package1");

    CHECK(p1->isConsumedBy(QString{"//..."}));
    CHECK(p1->isConsumedBy(QString{"//some/..."}));
    CHECK(p1->isConsumedBy(QString{"//some/path/..."}));

    CHECK_FALSE(p1->isConsumedBy(QString{"//some/path/sub-package1/..."}));
  }

  {
    auto p2 = root->addSubDirectories(QString{"/some/path/sub-package2"});
    REQUIRE(p2);

    auto somePath = root->findSubPackage(QString{"/some/path"});
    REQUIRE(somePath);
    CHECK(somePath->dirPath() == QString{"/some/path"});
    CHECK(somePath->subDirectories().size() == 2);

    CHECK(p2->name() == "sub-package2");
    CHECK(p2->dirPath() == QString{"/some/path/sub-package2"});
    CHECK(p2->bazelPath() == "//some/path/sub-package2");

    CHECK(p2->isConsumedBy(QString{"//..."}));
    CHECK(p2->isConsumedBy(QString{"//some/..."}));
    CHECK(p2->isConsumedBy(QString{"//some/path/..."}));

    CHECK_FALSE(p2->isConsumedBy(QString{"//some/path/sub-package2/..."}));
  }
}

}  // namespace BazelProjectManager::Internal::tests
