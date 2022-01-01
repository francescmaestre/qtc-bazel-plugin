#pragma once

// std
#include <optional>
#include <regex>
#include <tuple>

// Qt
#include <QString>

// Bazel
#include <3rd_party/bazel/src/main/protobuf/build.pb.h>


namespace BazelProjectManager::Internal {

/// Helps to parse Bazel label strings into components.
/// NB: This class makes no data copies and only returns references to the original string!
/// This means the source data must outlive instances of this class.
struct BazelLabel {
  static std::optional<BazelLabel> parse(const std::string& label);

  std::ssub_match repo() const { return matchResults_[1]; }
  std::ssub_match packageDirPath() const { return matchResults_[2]; }
  std::ssub_match targetPath() const { return matchResults_[4]; }

private:
  explicit BazelLabel(std::smatch matchResults);

  std::smatch matchResults_;
};


/// Models a Bazel project's structure in terms of packages and their build targets.
/// This is the simplest form needed to display e.g. a build step configuration UI.
struct BazelPackage {
  // Since children hold pointers to their parents we don't want the later to ever change their
  // addresses due to vector reallocation.
  using ChildrenContainerType = std::vector<std::shared_ptr<BazelPackage>>;

  using TargetsContainerType = std::vector<QString>;

  BazelPackage();

  BazelPackage(
    QString name,
    const BazelPackage* parentPackage,
    ChildrenContainerType subPackages,
    TargetsContainerType targets
  );

  BazelPackage subPackage(QString subPackageName) const;

  /// @param path - bazel label (starting with `//`) or a directory path (starting with `/`).
  /// @returns whether this package is covered by the wildcard `path` and is a child of `path`.
  /// E.g. //foo/bar is under //...
  /// But //foo is not under //foo/... - it's the wildcard parent itself.
  bool isConsumedBy(const QString& path) const;

  QString name;
  const BazelPackage* parentPackage = nullptr;
  ChildrenContainerType subPackages;
  TargetsContainerType targets;

  /// @returns package directory path, starting with "/".
  QString dirPath() const;

  /// @returns Bazel package path, starting with "//".
  QString bazelPath() const;

  /// @returns Bazel target label.
  QString targetLabel(const QString& targetPath) const;
};


/// Run a Bazel query against the project in the given workspace directory.
///
/// @param workspaceDir - Directory containing the Bazel workspace to query.
/// @param query - Query string to execute.
/// @see https://docs.bazel.build/versions/main/user-manual.html#query
std::tuple<int, blaze_query::QueryResult> bazelQuery(
  const QString& workspaceDir, const QString& query
);


/// Runs a query to list all rule targets of a given Bazel package.
///
/// @param workspaceDir - Directory containing the Bazel workspace to query.
/// @param packageDirPath - path, without the leading `//`, to the package to get the rules from.
std::tuple<int, blaze_query::QueryResult> queryPackageRules(
  const QString& workspaceDir, const QString& packageDirPath
);

}  // namespace BazelProjectManager::Internal
