#pragma once

// std
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <tuple>

// Qt
#include <QString>
#include <QFileInfo>

// Bazel
#include <3rd_party/bazel/src/main/protobuf/build.pb.h>

// own
#include <bazel_api_export.h>


namespace BazelProjectManager::Internal {

/// Helps to parse Bazel label strings into components.
/// NB: This class makes no data copies and only returns references to the original string!
/// This means the source data must outlive instances of this class.
struct BAZEL_API_EXPORT BazelLabel {
  static std::optional<BazelLabel> parse(const std::string& label);

  /// Repository spec. Starts with '@'!
  std::string_view repo() const;
  QByteArrayView repoBA() const;

  /// Path from the workspace root to the package directory. Starts with '/'.
  std::string_view packageDirPath() const;
  QByteArrayView packageDirPathBA() const;

  /// Target's immediate parent directory name. Starts with '/'.
  std::string_view targetParentDirName() const;
  QByteArrayView targetParentDirNameBA() const;

  /// Target unqualified name.
  /// NOTE: May contain '/' inside!
  std::string_view targetName() const;
  QByteArrayView targetNameBA() const;

private:
  explicit BazelLabel(std::smatch matchResults);

  std::smatch matchResults_;
};


/// Contains references to interesting attributes of Bazel rules.
/// WARNING: This struct is NON-OWNING and stores mostly just references!
struct BAZEL_API_EXPORT RuleAttributeRefs {
private:
  using StringValueListType = std::remove_reference_t<
    decltype(std::declval<blaze_query::Attribute>().string_list_value())
  >;

public:
  RuleAttributeRefs(const blaze_query::Rule& rule);

  bool is_executable = false;
};


/// Run a Bazel query against the project in the given workspace directory.
///
/// @param workspaceDir - Directory containing the Bazel workspace to query.
/// @param query - Query string to execute.
/// @see https://docs.bazel.build/versions/main/user-manual.html#query
blaze_query::QueryResult bazelQuery(
  const QString& workspaceDir, const QString& query
);


/// Runs a query to list all rule targets of a given Bazel package.
///
/// @param workspaceDir - Directory containing the Bazel workspace to query.
/// @param packageDirPath - path, without the leading `//`, to the package to get the rules from.
blaze_query::QueryResult queryPackageRules(
  const QString& workspaceDir, const QString& packageDirPath
);

}  // namespace BazelProjectManager::Internal
