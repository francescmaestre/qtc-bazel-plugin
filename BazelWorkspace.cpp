#include "BazelWorkspace.h"
#include "bazel_helpers.h"

// Qt
#include <QDir>

// own
#include "logging.h"

namespace BazelProjectManager::Internal {

namespace {

/// Prepare build target description for the IDE.
///
/// @param bazelRule - rule to process.
/// @param part - project part corresponding to the tule target.
ProjectExplorer::BuildTargetInfo createBuildTarget(
  const blaze_query::Rule& bazelRule,
  const ProjectExplorer::RawProjectPart& part,
  const Utils::FilePath& workspaceDirPath
) {
  using namespace ProjectExplorer;

  const auto& maybeParsedName = BazelLabel::parse(bazelRule.name());
  if (!maybeParsedName)
    throw std::runtime_error{"Could not parse target's label: " + bazelRule.name()};

  ProjectExplorer::BuildTargetInfo targetInfo;
  targetInfo.buildKey = part.buildSystemTarget;
  targetInfo.displayName = part.displayName;
  targetInfo.projectFilePath = Utils::FilePath::fromString(part.projectFile);
  // Runnable targets shall be picked up by the IDE and presented in the run menu for selection.
  targetInfo.isQtcRunnable = part.buildTargetType == BuildTargetType::Executable;
  if (bazelRule.rule_output_size()) {
    const auto& outputLabel = bazelRule.rule_output(0);  // We hope this is always the executable.
    auto maybeParsedLabel = BazelLabel::parse(outputLabel);
    if (!maybeParsedLabel) {
      // TODO: qCWarning(BazelPluginLog) << "Unrecognized target output: " << outputLabel.c_str();
    }
    else {
      // FIXME: This changes depending on the Bazel compilation mode (or, in our terms, the build
      // configuration type) and has to either be updated whenever the IDE switches between build
      // configurations, or the project model has to be kept in multiple instances - again, per
      // build configuration instance.
      targetInfo.targetFilePath =
        workspaceDirPath
        .pathAppended("bazel-bin")
        .pathAppended(QString::fromUtf8(maybeParsedLabel->packageDirPathBA()))
        .pathAppended(QString::fromUtf8(maybeParsedLabel->targetNameBA()));
    }
  }
  if (targetInfo.isQtcRunnable && !targetInfo.targetFilePath.isEmpty()) {
    targetInfo.workingDirectory = workspaceDirPath.pathAppended(
      QString::fromUtf8(maybeParsedName->packageDirPathBA())
    );
  }

  return targetInfo;
}  // createBuildTarget

/// Creates code model information for a given Bazel rule target.
ProjectExplorer::RawProjectPart createProjectPart(
  const blaze_query::Rule& bazelRule, const Utils::FilePath& workspaceDirPath
) {
  using namespace ProjectExplorer;

  const auto& maybeParsedName = BazelLabel::parse(bazelRule.name());
  if (!maybeParsedName)
    throw std::runtime_error{"Could not parse target's label: " + bazelRule.name()};

  const RuleAttributeRefs attrRefs{bazelRule};

  ProjectExplorer::RawProjectPart part;

  const auto& locationComponents = QString::fromStdString(bazelRule.location()).split(":");
  part.setProjectFileLocation(
    locationComponents.at(0),
    locationComponents.size() > 1 ? locationComponents.at(1).toInt() : -1,
    locationComponents.size() > 2 ? locationComponents.at(2).toInt() : -1
    );
  part.buildSystemTarget = QString::fromStdString(bazelRule.name());
  part.displayName = QString::fromUtf8(maybeParsedName->targetNameBA());
  part.buildTargetType = attrRefs.is_executable
    ? ProjectExplorer::BuildTargetType::Executable
    : ProjectExplorer::BuildTargetType::Unknown;  // TODO: Would be nice to distinguish libraries.

  // Bazel's convention is to always export include paths relative to the workspace root.
  part.headerPaths << HeaderPath{workspaceDirPath.toString(), HeaderPathType::User};
  // TODO: part.projectMacros = ...
  // TODO: part.flagsForC = ...
  // TODO: part.flagsForCxx = ...

  // Collect input sources.
  for (int i = 0; i < bazelRule.rule_input_size(); ++i) {
    const auto& inputLabel = bazelRule.rule_input(i);
    auto maybeParsedLabel = BazelLabel::parse(inputLabel);
    if (!maybeParsedLabel) {
      //qCWarning(BazelPluginLog) << "Unrecognized target input: " << inputLabel.c_str();
      continue;
    }

    if (maybeParsedLabel->repo().length()) {
      continue;  // TODO: Or can there also be source files from external repos?
    }

    const QString packageDirPath = QString::fromUtf8(maybeParsedLabel->packageDirPathBA());
    const QString relFilePath = QString::fromUtf8(maybeParsedLabel->targetNameBA());

    const Utils::FilePath absFilePath = workspaceDirPath/packageDirPath/relFilePath;
    if (!absFilePath.exists()) {
      continue;  // This way we filter out inputs which are non-files, or are non-existent.
    }

    part.files.push_back(absFilePath.toString());
  }  // for

  return part;
}


}  // namespace

bool operator<(const BuildTarget& left, const BuildTarget& right) {
  return left.buildTargetInfo.buildKey < right.buildTargetInfo.buildKey;
}


// --- BazelWorkspace ---


BazelWorkspace::BazelWorkspace(Utils::FilePath workspaceDirPath)
  : workspaceDirPath_{std::move(workspaceDirPath)},
    // FIXME: What if workspace dir does not contain a package?
    rootPackage_{std::make_shared<ProjectSubDirectory>(this)},
    queryStart_{std::chrono::steady_clock::now()},
    rulesQueryResult_{queryPackageRules(workspaceDirPath_.toString(), "...")}
{
  {
    const auto doneQuery = std::chrono::steady_clock::now();
    const auto queryDurationMillis =
      std::chrono::duration_cast<std::chrono::milliseconds>(doneQuery - queryStart_);
    qCInfo(BazelPluginLog) << "Querying duration:" << queryDurationMillis.count() << "ms";
  }

  const auto collectStart = std::chrono::steady_clock::now();
  collectBazelTargets();  // 146 - 196 ms
  const auto collectEnd = std::chrono::steady_clock::now();
  const auto collectDurationMillis =
    std::chrono::duration_cast<std::chrono::milliseconds>(collectEnd - collectStart);
  qCInfo(BazelPluginLog) << "Target collection:" << collectDurationMillis.count() << "ms";
}

bool BazelWorkspace::isKnownSourceFile(const Utils::FilePath& fileAbsPath) const {
  return knownSources_.find(fileAbsPath) != knownSources_.end();
}

void BazelWorkspace::addToKnownSources(const ProjectExplorer::RawProjectPart& part) {
  for (const auto& partFile : part.files) {
    knownSources_.insert(Utils::FilePath::fromString(partFile));
  }
}

ProjectExplorer::RawProjectParts BazelWorkspace::collectProjectParts() const {
  ProjectExplorer::RawProjectParts result;
  result.push_back(stubPart_);

  using FillFuncType = std::function<void(const ProjectSubDirectory*)>;
  const FillFuncType fillProjectParts = [&](const ProjectSubDirectory* dir) {
    for (const auto& target : dir->targets()) {
      result.push_back(target.projectPart);
    }

    for (const auto& [name, directory] : dir->subDirectories()) {
      fillProjectParts(directory.get());
    }
  };
  fillProjectParts(rootPackage().get());

  return result;
}

QVector<ProjectExplorer::BuildTargetInfo>
BazelWorkspace::collectBuildTargets(const BuildTargetKind kind) const
{
  QVector<ProjectExplorer::BuildTargetInfo> result;

  std::function<void(const ProjectSubDirectory*)> fillRunnableTargets =
    [&](const ProjectSubDirectory* dir) {
      for (const auto& t : dir->targets()) {
        if (kind == BuildTargetKind::OnlyRunnable && !t.buildTargetInfo.isQtcRunnable)
          continue;
        result.push_back(t.buildTargetInfo);
      }

      for (const auto& p : dir->subDirectories()) {
        fillRunnableTargets(p.second.get());
      }
    };
  fillRunnableTargets(rootPackage().get());

  return result;
}

void BazelWorkspace::collectBazelTargets() {
  const auto n_targets = rulesQueryResult_.target_size();

  for (int i = 0; i < n_targets; i++) {
    const auto& bazelTarget = rulesQueryResult_.target(i);
    if (bazelTarget.type() != blaze_query::Target_Discriminator_RULE) {
      continue;
    }
    const blaze_query::Rule& bazelRule = bazelTarget.rule();

    const auto& maybeParsedName = BazelLabel::parse(bazelRule.name());
    if (!maybeParsedName) {
      throw std::runtime_error{"Could not parse target's label: " + bazelRule.name()};
    }

    auto package = rootPackage_->addSubDirectories(
      QString::fromUtf8(maybeParsedName->packageDirPathBA())
    );
    Q_ASSERT(package);
    package->placeTarget(bazelRule);
  }  // for
}

// --- ProjectSubDirectory ---

ProjectSubDirectory::ProjectSubDirectory(BazelWorkspace* workspace)
: workspace_{workspace}
{}

ProjectSubDirectory::ProjectSubDirectory(const QStringView name)
: name_{name.toString()}
{
}

const QString& ProjectSubDirectory::name() const {
  return name_;
}

bool ProjectSubDirectory::isConsumedBy(const QStringView path) const {
  constexpr auto ellipsisLen = 3;
  static const QString subdirWildcardExpr = "/...";
  static const QString rootPackageExpr = "//";

  if (!path.endsWith(subdirWildcardExpr))
    return false;

  const auto& wildcardParentPath = path.left(path.length() - ellipsisLen);
  const auto& selfPath = wildcardParentPath.startsWith(rootPackageExpr) ? bazelPath() : dirPath();
  return selfPath.startsWith(wildcardParentPath) && selfPath.length() > wildcardParentPath.length();
}

QStringView ProjectSubDirectory::dirPath() const {
  return QStringView{bazelPath()}.sliced(1);
}

const QString& ProjectSubDirectory::bazelPath() const {
  if (cachedBazelPath_.isEmpty()) {
    auto lockedParent = parentPackage_.lock();
    if (lockedParent) {
      const auto& parentPath = lockedParent->dirPath();
      cachedBazelPath_ = "/" + parentPath + (parentPath.endsWith('/') ? "" : "/") + name();
    }
    else {
      cachedBazelPath_ = "//";  // root package name must always be "//".
    }
  }
  return cachedBazelPath_;
}

BazelWorkspace* ProjectSubDirectory::workspace() const {
  if (workspace_)
    return workspace_;

  auto lockedParent = parentPackage_.lock();
  Q_ASSERT(lockedParent);
  return lockedParent->workspace();
}

Utils::FilePath ProjectSubDirectory::workspaceDirPath() const {
  return workspace()->workspaceDirPath();
}

void ProjectSubDirectory::addSubDir(std::shared_ptr<ProjectSubDirectory> child) {
  subDirs_.emplace(child->name(), child);
  child->parentPackage_ = shared_from_this();
}

std::shared_ptr<ProjectSubDirectory>
ProjectSubDirectory::addSubDirectories(const QStringView subDirPath) {
  std::shared_ptr<ProjectSubDirectory> lastAddedDir = shared_from_this();

  for (const QStringView subdirName : subDirPath.split('/', Qt::SkipEmptyParts)) {
    std::shared_ptr<ProjectSubDirectory> subdir = lastAddedDir->findSubPackage(subdirName);
    if (!subdir) {
      subdir.reset(new ProjectSubDirectory(subdirName));
      lastAddedDir->addSubDir(subdir);
    }
    lastAddedDir = subdir;
  }

  return lastAddedDir;
}

std::shared_ptr<ProjectSubDirectory>
ProjectSubDirectory::findSubPackage(const QStringView path) {
  auto searchedParent = shared_from_this();
  for (const auto subdirName : path.split('/', Qt::SkipEmptyParts)) {
    // TODO: See if using string view as key is possible.
    auto foundIter = searchedParent->subDirs_.find(subdirName.toString());
    if (foundIter == searchedParent->subDirs_.end())
      return nullptr;
    searchedParent = foundIter->second;
  }
  // If we made it here, every path element lookup was successfull.
  return searchedParent;
}

void ProjectSubDirectory::placeTarget(const blaze_query::Rule& bazelRule) {
  auto projectPart = createProjectPart(bazelRule, workspaceDirPath());
  // FIXME: Populating "known sources" should be reimplemented.
  workspace()->addToKnownSources(projectPart);

  auto buildTargetInfo = createBuildTarget(bazelRule, projectPart, workspaceDirPath());
  bazelTargets_.insert(BuildTarget{std::move(projectPart), std::move(buildTargetInfo)});
}

}  // namespace BazelProjectManager::Internal
