#pragma once

#include <tuple>

#include <QString>
#include <3rd_party/bazel/src/main/protobuf/build.pb.h>


namespace BazelProjectManager::Internal {

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
/// @param packagePath - path, without the leading `//`, to the package to get the rules from.
std::tuple<int, blaze_query::QueryResult> queryPackageRules(
  const QString& workspaceDir, const QString& packagePath
);

}  // namespace BazelProjectManager::Internal
