#include "bazel_helpers.h"

#include <memory>

#include <QProcess>


namespace BazelProjectManager::Internal {

// --- BazelLabel ---

std::optional<BazelLabel> BazelLabel::parse(const std::string& label) {
  // TODO: Verify against actual Starlark syntax rules.
  static const std::regex labelRe{"^(@\\S+)?/((/[^/:]+)*):(([^/:]+/)*[^/:]+)$"};
  std::smatch matchResults;
  if (!std::regex_match(label, matchResults, labelRe)) {
    return std::nullopt;
  }
  return BazelLabel{std::move(matchResults)};
}

BazelLabel::BazelLabel(std::smatch matchResults)
: matchResults_{std::move(matchResults)}
{}


// --- BazelPackage ---

BazelPackage::BazelPackage()
  : BazelPackage("/", nullptr, {}, {})
{}

BazelPackage::BazelPackage(
  QString name,
  const BazelPackage* parentPackage,
  ChildrenContainerType subPackages,
  TargetsContainerType targets
)
  : name{std::move(name)},
    parentPackage{parentPackage},
    subPackages{std::move(subPackages)},
targets{std::move(targets)}
{}

BazelPackage BazelPackage::subPackage(QString subPackageName) const {
  return BazelPackage{std::move(subPackageName), this, {}, {}};
}

bool BazelPackage::isConsumedBy(const QString& path) const {
  static const QString subdirWildcard = "/...";
  if (!path.endsWith(subdirWildcard))
    return false;

  const auto& wildcardParentPath = path.leftRef(path.length() - 3);
  const auto& selfPath = wildcardParentPath.startsWith("//") ? bazelPath() : dirPath();
  return selfPath.startsWith(wildcardParentPath) && selfPath.length() > wildcardParentPath.length();
}

QString BazelPackage::dirPath() const {
  if (!parentPackage)
    return name;  // root pakage name should be "/".
  const auto& parentPath = parentPackage->dirPath();
  return parentPath + (parentPath.endsWith("/") ? "" : "/") + name;
  // TODO: Cache the result?
}

QString BazelPackage::bazelPath() const {
  return "/" + dirPath();
}

QString BazelPackage::targetLabel(const QString& targetName) const {
  return bazelPath() + ":" + targetName;
}


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
  const QString& workspaceDir, const QString& packageDirPath
) {
  return bazelQuery(workspaceDir, QString("kind(rule, //%1:*)").arg(packageDirPath));
}

}  // namespace BazelProjectManager::Internal
