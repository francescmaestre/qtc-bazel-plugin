#pragma once

#include <string_view>

namespace blaze_query {
class QueryResult;
}


namespace BazelProjectManager::Internal {

/// Run a Bazel query against the project in the current working directory (if any).
/// @param query - Query string to run.
/// @see https://docs.bazel.build/versions/main/user-manual.html#query
blaze_query::QueryResult bazelQuery(const std::string_view query);

}  // namespace BazelProjectManager::Internal
