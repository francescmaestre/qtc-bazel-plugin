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
#include <bazel/src/main/protobuf/build.pb.h>


namespace BazelProjectManager::Internal {

/// Helps to parse Bazel label strings into components.
/// NB: This class makes no data copies and only returns references to the original string!
/// This means the source data must outlive instances of this class.
struct BazelLabel {
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
struct RuleAttributeRefs {
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


enum class BazelCompilationMode: char{
  Fast,
  Dbg,
  Opt,

  CompileMode_LAST
};


/// @returns CLI argument value corresponding to the compilation mode.
QStringView compileModeToCLIArg(BazelCompilationMode mode);


enum class QueryTargetKind: char {
  Rule          = 1 << ::blaze_query::Target::Discriminator::Target_Discriminator_RULE,
  SourceFile    = 1 << ::blaze_query::Target::Discriminator::Target_Discriminator_SOURCE_FILE,
  GeneratedFile = 1 << ::blaze_query::Target::Discriminator::Target_Discriminator_GENERATED_FILE,
  PackageGroup  = 1 << ::blaze_query::Target::Discriminator::Target_Discriminator_PACKAGE_GROUP,
  EnvGroup      = 1 << ::blaze_query::Target::Discriminator::Target_Discriminator_ENVIRONMENT_GROUP,

  // Combined shortcut values:
  FilesAndRules = Rule | SourceFile | GeneratedFile,
  Groups        = PackageGroup | EnvGroup,
  AllKinds      = FilesAndRules | Groups,
};

/// Runs a query to list all targets of a specified kind of a given Bazel package.
///
/// @param workspaceDir - Directory containing the Bazel workspace to query.
/// @param packageDirPath - path, without the leading `//`, to the package to get the rules from.
///        Use '...' to query all subpackages.
/// @param targetKinds - kinds of targets to include into the result.
/// @returns Protobuf structure containing the query result.
///
/// The query uses the `--order_output=deps` flag, meaning dependencies are coming first.
/// @see https://bazel.build/query/language#results-ordering.
/// @note There might be a bug, but Bazel does not really order source files as dependencies of the
/// rules using them as inputs.
blaze_query::QueryResult queryPackage(
    const QString& workspaceDir,
    const QString& packageDirPath,
    const QueryTargetKind targetKinds = QueryTargetKind::AllKinds
);

}  // namespace BazelProjectManager::Internal
