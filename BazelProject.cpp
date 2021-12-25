#include "BazelProject.h"

#include <coreplugin/icontext.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/projectnodes.h>
#include <utils/filepath.h>
#include <utils/runextensions.h>

#include "BazelBuildSystem.h"
#include "plugin_constants.h"
#include "bazel_helpers.h"
#include "logging.h"


namespace BazelProjectManager::Internal {

namespace {

const char BAZEL_PACKAGE_BUILD_FILE_NAME[] = "BUILD";

/// Create appropriate project nodes out of a Bazel rule item.
///
/// @param bazelRule - rule to process.
/// @param parentFolder [out] - project folder to append new target node to.
/// @param buildTargets [out] - output list to append the build target info to.
/// @param knownSources [out] - output set to insert the target's source paths into.
void processBazelRule(
  const blaze_query::Rule& bazelRule,
  ProjectExplorer::FolderNode* parentFolder,
  QList<ProjectExplorer::BuildTargetInfo>& buildTargets,
  std::set<Utils::FilePath>& knownSources
) {
  // Prepare target description
  {
    ProjectExplorer::BuildTargetInfo targetInfo{};
    targetInfo.buildKey = QString::fromStdString(bazelRule.name());
    targetInfo.displayName = targetInfo.buildKey.split(":").back();  // Un-qualified target name.
    targetInfo.projectFilePath =
      Utils::FilePath::fromString(QString::fromStdString(bazelRule.location()));  // TODO: Strip ':line:column'
    targetInfo.workingDirectory = parentFolder->filePath();
    if (bazelRule.rule_output_size()) {
      const auto& path = bazelRule.rule_output(0);
      targetInfo.targetFilePath = Utils::FilePath::fromUtf8(path.data(), path.size());
    }
    buildTargets.push_back(std::move(targetInfo));
  }

  const ProjectExplorer::BuildTargetInfo& buildTarget = buildTargets.back();

  auto targetNode = std::make_unique<ProjectExplorer::ProjectNode>(
    Utils::FilePath::fromString(buildTarget.displayName)
  );
  // Make this appear differently, not like a normal directory.
  targetNode->setIcon(":/projectexplorer/images/build.png");

  // Collect input sources.
  {
    for (int i = 0; i < bazelRule.rule_input_size(); ++i) {
      const auto& inputLabel = bazelRule.rule_input(i);
      const auto inputLabelQS = QString::fromUtf8(inputLabel.data(), inputLabel.size());
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
      auto fileNode = std::make_unique<ProjectExplorer::FileNode>(
        fileAbsPath,
        ProjectExplorer::FileType::Source
      );
      targetNode->addNode(std::move(fileNode));
      knownSources.insert(fileAbsPath);
    }  // for
  }

  parentFolder->addNode(std::move(targetNode));
}  // processBazelRule

}  // namespace BazelProjectManager::Internal


class BazelProject::ProjectScanner : public QObject {
public:
  ProjectScanner(Utils::FilePath projectFilePath)
    : projectFilePath_{std::move(projectFilePath)} {
  }

  void startAsync() {
    appTargets_.clear();
    targetSources_.clear();
    rootNode_ = std::make_unique<ProjectExplorer::ProjectNode>(workspaceDirPath());

    Utils::runAsync(
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

  // Since there's only one possible caller of these, we just let take the ownership.

  std::unique_ptr<ProjectExplorer::ProjectNode> takeRootNode() {
    return std::move(rootNode_);
  }

  QList<ProjectExplorer::BuildTargetInfo> takeTargets() {
    return std::move(appTargets_);
  }

signals:
  void scanComplete(bool good);

private:
  Utils::FilePath workspaceDirPath() const { return projectFilePath_.parentDir(); }

  QDir workspaceDir() const { return workspaceDirPath().toDir(); }

  void rescanProject(ProjectExplorer::FolderNode* rootNode) {
    QDir rootDir = rootNode->path();

    if (rootDir.exists(BAZEL_PACKAGE_BUILD_FILE_NAME)) {  // This is a Bazel package root.
      // TODO: Add overlay icong to the current folder node.

      const auto fileAbsPath = rootNode->filePath().pathAppended(BAZEL_PACKAGE_BUILD_FILE_NAME);
      // TODO: Add overlay icong to the BUILD file.
      rootNode->addNode(std::make_unique<ProjectExplorer::FileNode>(
        fileAbsPath,
        ProjectExplorer::FileType::Project
      ));
      targetSources_.insert(fileAbsPath);

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
        processBazelRule(bazelTarget.rule(), rootNode, appTargets_, targetSources_);
      }  // for
    }  // if (rootDir.exists(BAZEL_PACKAGE_BUILD_FILE_NAME))

    // List files not belonging to any build target.
    // TODO: Handle WORKSPACE files specially: mark as FileType::Project and add a custom icon.
    const auto& fileNames = rootDir.entryList(QDir::Files, QDir::Name);
    for (const auto& fileName : fileNames) {
      const auto fileAbsPath = rootNode->filePath().pathAppended(fileName);
      if (targetSources_.find(fileAbsPath) != targetSources_.cend()) {
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
  }  // rebuildProjectStructure

  Q_OBJECT

  Utils::FilePath projectFilePath_;

  std::set<Utils::FilePath> targetSources_;
  std::unique_ptr<ProjectExplorer::ProjectNode> rootNode_;
  QList<ProjectExplorer::BuildTargetInfo> appTargets_;
};  // class ProjectScanner


// BazelProject public

BazelProject::BazelProject(const Utils::FilePath& fileName)
  : ProjectExplorer::Project(Constants::Project::MIMETYPE, fileName)
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
  if (!parserMutex_.try_lock()) {
    return;
  }
  scanner_->startAsync();
}

void BazelProject::onScanComplete(bool good) {
  setRootProjectNode(scanner_->takeRootNode());
  targets_ = scanner_->takeTargets();
  emit projectScanComplete(good);

  parserMutex_.unlock();
}

}  // namespace BazelProjectManager::Internal


#include "BazelProject.moc"
