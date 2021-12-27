#include "BazelProject.h"

#include <coreplugin/icontext.h>
#include <cpptools/cppprojectupdater.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/rawprojectpart.h>
#include <projectexplorer/target.h>
#include <utils/filepath.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include "BazelBuildSystem.h"
#include "plugin_constants.h"
#include "bazel_helpers.h"
#include "logging.h"


namespace BazelProjectManager::Internal {

namespace {

// NOTE: WORKSPACE file name is declared in the plugin's JSON manifest file.

const char BAZEL_PACKAGE_BUILD_FILE_NAME[] = "BUILD";
const char BAZEL_PACKAGE_BUILD_FILE_NAME_W_EXT[] = "BUILD.bazel";

}  // namespace BazelProjectManager::Internal


class BazelProject::ProjectScanner : public QObject {
  Q_OBJECT

public:
  ProjectScanner(Utils::FilePath projectFilePath)
    : projectFilePath_{std::move(projectFilePath)} {
  }

  void startAsync();

  // Since there's only one possible caller of these, we just let take the ownership.

  std::unique_ptr<ProjectExplorer::ProjectNode> takeRootNode() {
    return std::exchange(rootNode_, {});
  }

  QList<ProjectExplorer::BuildTargetInfo> takeTargets() {
    return std::exchange(appTargets_, {});
  }

  /// RawProjectParts holds most of a project's C++ code model: inputs, targets, includes.
  ProjectExplorer::RawProjectParts takeParts() {
    return std::exchange(parts_, {});
  }

signals:
  void scanComplete(bool good);

private:
  Utils::FilePath workspaceDirPath() const { return projectFilePath_.parentDir(); }

  QDir workspaceDir() const { return workspaceDirPath().toDir(); }

  void rescanProject(ProjectExplorer::FolderNode* rootNode);  // rebuildProjectStructure


  /// Create appropriate project nodes and code model info out of a Bazel rule item.
  ///
  /// @param bazelRule - rule to process.
  /// @param parentFolder [out] - project folder to append new target node to.
  void processBazelRule(
    const blaze_query::Rule& bazelRule,
    ProjectExplorer::FolderNode* parentFolder
  );  // processBazelRule


  Utils::FilePath projectFilePath_;

  std::set<Utils::FilePath> knownSources_;
  std::unique_ptr<ProjectExplorer::ProjectNode> rootNode_;
  QList<ProjectExplorer::BuildTargetInfo> appTargets_;
  ProjectExplorer::RawProjectParts parts_;
};  // class ProjectScanner

void BazelProject::ProjectScanner::startAsync() {
  appTargets_.clear();
  knownSources_.clear();
  parts_.clear();
  rootNode_ = std::make_unique<ProjectExplorer::ProjectNode>(workspaceDirPath());

  Utils::runAsync(
  ProjectExplorer::ProjectExplorerPlugin::sharedThreadPool(),
  [this]() {
    try {
      rescanProject(rootNode_.get());
    }
    catch(const std::exception& e) {
      emit scanComplete(false);
      qCWarning(BazelPluginLog) << "Project scan failed: " << e.what();
    }
    catch(...) {
      emit scanComplete(false);
      qCWarning(BazelPluginLog) << "Project scan failed for unknown reason.";
    }

    emit scanComplete(true);
  }
  );
}

void BazelProject::ProjectScanner::rescanProject(ProjectExplorer::FolderNode* rootNode) {
  QDir rootDir = rootNode->path();

  const auto maybeBuildFilePath = [&rootNode]() -> std::optional<Utils::FilePath> {
    auto buildFilePath = rootNode->filePath().pathAppended(BAZEL_PACKAGE_BUILD_FILE_NAME);
    if (buildFilePath.exists())
      return std::move(buildFilePath);
    buildFilePath = rootNode->filePath().pathAppended(BAZEL_PACKAGE_BUILD_FILE_NAME_W_EXT);
    if (buildFilePath.exists())
      return std::move(buildFilePath);
    return std::nullopt;
  }();
  if (maybeBuildFilePath.has_value()) {  // This is a Bazel package root.
    // TODO: Add overlay icong to the current folder node.
    // TODO: Add overlay icong to the BUILD file.
    rootNode->addNode(std::make_unique<ProjectExplorer::FileNode>(
      *maybeBuildFilePath,
      ProjectExplorer::FileType::Project
    ));
    knownSources_.insert(*maybeBuildFilePath);

    const auto& packagePath = workspaceDir().relativeFilePath(rootDir.path());
    const auto& [exitCode, qr] = queryPackageRules(workspaceDirPath().toString(), packagePath);

    const auto n_targets = qr.target_size();
    qCDebug(BazelPluginLog)
      << "Bazel package " << packagePath << " has " << n_targets << " targets";

    for (int i = 0; i < n_targets; i++) {
      const auto& bazelTarget = qr.target(i);
      if (bazelTarget.type() != blaze_query::Target_Discriminator_RULE) {
        continue;
      }
      processBazelRule(bazelTarget.rule(), rootNode);
    }  // for
  }  // if (rootDir.exists(BAZEL_PACKAGE_BUILD_FILE_NAME))

  // List files not belonging to any build target.
  // TODO: Handle WORKSPACE files specially: mark as FileType::Project and add a custom icon.
  const auto& fileNames = rootDir.entryList(QDir::Files, QDir::Name);
  for (const auto& fileName : fileNames) {
    const auto fileAbsPath = rootNode->filePath().pathAppended(fileName);
    if (knownSources_.find(fileAbsPath) != knownSources_.cend()) {
      continue;  // Skip those belonging to some target.
    }
    rootNode->addNode(std::make_unique<ProjectExplorer::FileNode>(
      fileAbsPath,
      ProjectExplorer::FileType::Unknown
    ));
  }

  // Process subdirectories in the same way.
  const auto& subdirNames = rootDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot, QDir::Name);
  for (const auto& subdir : subdirNames) {
    // FIXME: Make up a more robust chek here.
    if (subdir.startsWith("bazel-")) {
      continue;  // This is one of Bazel's own build dirs. We don't want to go in there.
    }
    auto subdirNode = std::make_unique<ProjectExplorer::FolderNode>(
      Utils::FilePath::fromString(rootDir.filePath(subdir))
    );
    subdirNode->setDisplayName(subdir);
    rescanProject(subdirNode.get());
    rootNode->addNode(std::move(subdirNode));
  }
}

void BazelProject::ProjectScanner::processBazelRule(
  const blaze_query::Rule& bazelRule,
  ProjectExplorer::FolderNode* parentFolder
) {

  // Collect code model info.
  {
    ProjectExplorer::RawProjectPart part;

    const auto& locationComponents = QString::fromStdString(bazelRule.location()).split(":");
    part.setProjectFileLocation(
      locationComponents.at(0),
      locationComponents.size() > 1 ? locationComponents.at(1).toInt() : -1,
      locationComponents.size() > 2 ? locationComponents.at(2).toInt() : -1
    );
    part.buildSystemTarget = QString::fromStdString(bazelRule.name());
    part.displayName = part.buildSystemTarget.split(":").back();  // Un-qualified target name.

    // FIXME: part.buildTargetType = ...
    // FIXME: part.headerPaths = ...
    // FIXME: part.projectMacros = ...
    // FIXME: part.flagsForC = ...
    // FIXME: part.flagsForCxx = ...

    // Collect input sources.
    for (int i = 0; i < bazelRule.rule_input_size(); ++i) {
      const auto inputLabelQS = QString::fromStdString(bazelRule.rule_input(i));
      if (!inputLabelQS.startsWith("//")) {
        continue;  // Ignore external labels or anything that is definitely not a file.
        // FIXME: This may still be a label pointing to another target, not a source file.
        // E.g.:
        // "//lib:hello-time",
        // "//main:hello-greet",
        // "//main:hello-world.cc",
      }
      const auto fileName = inputLabelQS.split(":").back();
      const auto fileAbsPath = parentFolder->filePath().pathAppended(fileName);
      knownSources_.insert(fileAbsPath);

      part.files.push_back(fileAbsPath.toString());
    }  // for

    parts_.push_back(std::move(part));
  }
  const ProjectExplorer::RawProjectPart& part = parts_.back();

  // Prepare build target description.
  {
    ProjectExplorer::BuildTargetInfo targetInfo{};
    targetInfo.buildKey = part.buildSystemTarget;
    targetInfo.displayName = part.displayName;
    targetInfo.projectFilePath = Utils::FilePath::fromString(part.projectFile);
    targetInfo.workingDirectory = parentFolder->filePath();
    if (bazelRule.rule_output_size()) {
      const auto& path = bazelRule.rule_output(0);
      targetInfo.targetFilePath = Utils::FilePath::fromUtf8(path.data(), path.size());
    }
    appTargets_.push_back(std::move(targetInfo));
  }
  const ProjectExplorer::BuildTargetInfo& buildTarget = appTargets_.back();

  // Create explorer tree nonde.
  {
    auto targetNode = std::make_unique<ProjectExplorer::ProjectNode>(
    Utils::FilePath::fromString(buildTarget.displayName)
    );
    // Make this appear differently, not like a normal directory.
    targetNode->setIcon(":/projectexplorer/images/build.png");

    for (const QString& fileAbsPath : part.files) {
      auto fileNode = std::make_unique<ProjectExplorer::FileNode>(
        Utils::FilePath::fromString(fileAbsPath),
        ProjectExplorer::FileType::Source
      );
      targetNode->addNode(std::move(fileNode));
    }  // for

    parentFolder->addNode(std::move(targetNode));
  }
}


// BazelProject public

BazelProject::BazelProject(const Utils::FilePath& fileName)
: ProjectExplorer::Project(Constants::Project::MIMETYPE, fileName),
cppCodeModelUpdater_{std::make_unique<CppTools::CppProjectUpdater>()}
{
  scanner_ = std::make_unique<ProjectScanner>(projectFilePath());
  connect(scanner_.get(), &ProjectScanner::scanComplete, this, &BazelProject::onScanComplete);

  setId(Constants::Project::ID);
  setDisplayName(projectDirectory().fileName());

  setProjectLanguages({
    ProjectExplorer::Constants::C_LANGUAGE_ID,
    ProjectExplorer::Constants::CXX_LANGUAGE_ID
  });

  setNeedsBuildConfigurations(true);
  setNeedsDeployConfigurations(false);
  setHasMakeInstallEquivalent(false);
  setCanBuildProducts();
  setBuildSystemCreator([](ProjectExplorer::Target* t) {
    // Yes, the IDE assumes ownership. See `~TargetPrivate` in projectexplorer/target.cpp.
    return new BazelBuildSystem(t);
  });

  startProjectStructureUpdate();
}

ProjectExplorer::DeploymentKnowledge BazelProject::deploymentKnowledge() const
{
  return ProjectExplorer::DeploymentKnowledge::Bad;
}

// private

QDir BazelProject::workspaceDir() const { return projectDirectory().toDir(); }

void BazelProject::startProjectStructureUpdate() {
  if (!scannerMutex_.try_lock()) {
    return;
  }
  scanner_->startAsync();
}

void BazelProject::onScanComplete(bool good) {
  std::scoped_lock<std::mutex> lock(std::adopt_lock, scannerMutex_);  // Unlock it no matter what.

  setRootProjectNode(scanner_->takeRootNode());
  targets_ = scanner_->takeTargets();

  emit projectScanComplete(good);

  if (!activeTarget())
    return;  // activeTarget() is null before adding one.

  // Update C++ code model.
  cppCodeModelUpdater_->update(
    ProjectExplorer::ProjectUpdateInfo{
      this,
      ProjectExplorer::KitInfo{activeTarget()->kit()},  // TODO: What if the active Target changes?
      activeTarget()->activeBuildConfiguration()->environment(),
      scanner_->takeParts()
    }
  );
}

}  // namespace BazelProjectManager::Internal


#include "BazelProject.moc"
