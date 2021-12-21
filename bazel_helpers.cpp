#include "bazel_helpers.h"

#include <memory>

#include <QProcess>


namespace BazelProjectManager::Internal {

std::tuple<int, blaze_query::QueryResult> bazelQuery(
  const QString& workspaceDir, const QString& query
) {
  QProcess bazelProc;
  bazelProc.setWorkingDirectory(workspaceDir);
  bazelProc.start("bazel", {"query", query, "--output", "proto"});
  bazelProc.waitForFinished();

  // TODO: Use some streaming instead of storing the entire output in memory.
  const auto& bazelOutput = bazelProc.readAllStandardOutput();
  blaze_query::QueryResult qr;
  if (!qr.ParseFromArray(bazelOutput.data(), bazelOutput.size())) {
    throw std::runtime_error("Could not parse bazel output.");
  }

  return {bazelProc.exitCode(), std::move(qr)};
}

std::tuple<int, blaze_query::QueryResult> queryPackageRules(
  const QString& workspaceDir, const QString& packagePath
) {
  return bazelQuery(workspaceDir, QString("kind(rule, //%1:*)").arg(packagePath));
}

}  // namespace BazelProjectManager::Internal
