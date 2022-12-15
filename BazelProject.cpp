#include "BazelProject.h"

#include <coreplugin/icontext.h>
#include <cppeditor/cppprojectupdater.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/rawprojectpart.h>
#include <projectexplorer/target.h>
#include <utils/filepath.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include "BazelBuildSystem.h"
#include "BazelWorkspace.h"
#include "bazel_helpers.h"
#include "plugin_constants.h"
#include "logging.h"


namespace BazelProjectManager::Internal {

namespace {
using namespace ProjectExplorer;

// NOTE: WORKSPACE file name is declared in the plugin's JSON manifest file.

const char BAZEL_PACKAGE_BUILD_FILE_NAME[] = "BUILD";
const char BAZEL_PACKAGE_BUILD_FILE_NAME_W_EXT[] = "BUILD.bazel";
const char BUILD_ICON[] = ":/projectexplorer/images/build.png";
}  // namespace


// --- ProjectScanner ---

class BazelProject::ProjectScanner : public QObject {
  Q_OBJECT

public:
  ProjectScanner(Utils::FilePath projectFilePath)
    : projectFilePath_{std::move(projectFilePath)} {
  }

  QFuture<void> startAsync();

  // Since there's only one possible caller of these, we just let take the ownership.

  std::unique_ptr<ProjectNode> takeRootProjectNode() {
    return std::exchange(rootProjectNode_, {});
  }

  std::unique_ptr<BazelWorkspace> takeWorkspace() {
    return std::exchange(workspace_, {});
  }

signals:
  void scanComplete(bool good);

private:
  Utils::FilePath workspaceDirPath() const { return projectFilePath_.parentDir(); }

  QDir workspaceDir() const { return QDir(workspaceDirPath().path()); }

  /// Query Bazel for build targets and process the results into a workable structure.
  ///
  /// @param destPackage - container for the discovered targets and sub-packages.
  /// @returns root package containing the project structure.
  std::unique_ptr<ProjectSubDirectory> collectBazelTargets();

  /// Recursively fills the child content under a given project explorer node.
  ///
  /// This will combine the information about build targets with the file system entries and
  /// populate the project explorer folder with relevant child nodes.
  ///
  /// @param folderNode - project folder corresponding to a real FS directory.
  void buildExplorerFolderContents(FolderNode* folderNode);


  Utils::FilePath projectFilePath_;

  std::unique_ptr<ProjectNode> rootProjectNode_;
  std::unique_ptr<BazelWorkspace> workspace_;

  QFutureInterface<void>* futureInterface_ = nullptr;
};  // class ProjectScanner


QFuture<void> BazelProject::ProjectScanner::startAsync() {
  workspace_.reset();
  rootProjectNode_ = std::make_unique<ProjectNode>(workspaceDirPath());

  return Utils::runAsync(
    ProjectExplorerPlugin::sharedThreadPool(),
    [this](QFutureInterface<void>& futureInterface) {
      futureInterface_ = &futureInterface;

      bool success = false;
      const auto beganProcessing = std::chrono::steady_clock::now();
      try {
        // This will query Bazel for all targets in the workspace and build a tree structure of
        // packages and targets.        
        workspace_ = std::make_unique<BazelWorkspace>(workspaceDirPath());

        const auto beganDirScan = std::chrono::steady_clock::now();
        buildExplorerFolderContents(rootProjectNode_.get());  // 77 - 131 ms
        success = true;
        const auto scanDurationMillis = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - beganDirScan
        );
        qCInfo(BazelPluginLog) << "Files scan duration:" << scanDurationMillis.count() << "ms";
      }
      catch(const std::exception& e) {
        qCWarning(BazelPluginLog) << "Project scan failed: " << e.what();
      }
      catch(...) {
        qCWarning(BazelPluginLog) << "Project scan failed for unknown reason.";
      }

      const auto doneProcessing = std::chrono::steady_clock::now();
      const auto parsingDurationMillis =
        std::chrono::duration_cast<std::chrono::milliseconds>(doneProcessing - beganProcessing);
      qCInfo(BazelPluginLog) << "Total duration:" << parsingDurationMillis.count() << "ms";

      futureInterface_ = nullptr;
      emit scanComplete(success);
    }
  );
}

void BazelProject::ProjectScanner::buildExplorerFolderContents(FolderNode* folderNode) {
  if (futureInterface_->isCanceled()) {
    return;
  }

  // Create explorer tree nodes for each build target declared in its BUILD file.
  const auto& folderRelativePath = folderNode->path().relativeChildPath(workspaceDirPath());
  const auto packageInThisFolder =
    workspace_->rootPackage()->findSubPackage(folderRelativePath.toString());
  if (packageInThisFolder) {
    // TODO: Add overlay icong to the folderNode.
    // TODO: Add overlay icong to the BUILD file.

    // List all targets and their input files.
    for (const auto& target : packageInThisFolder->targets()) {
      auto targetNode = std::make_unique<VirtualFolderNode>(folderNode->filePath());
      targetNode->setDisplayName(target.buildTargetInfo.displayName);
      targetNode->setIcon(BUILD_ICON);  // Make it appear differently, not like just a directory.

      for (const QString& fileAbsPath : target.projectPart.files) {
        auto fileNode = std::make_unique<FileNode>(
          Utils::FilePath::fromString(fileAbsPath),
          FileType::Source
        );
        // This will add intermediate folder nodes in case file is in a folderNode's subdirectory.
        targetNode->addNestedNode(std::move(fileNode));
      }  // for

      folderNode->addNode(std::move(targetNode));
    }
  }

  const QDir directory{folderNode->path().path()};

  // Check directory contents to make sure we're not hiding something potentially useful.
  const auto& fileNames = directory.entryList(QDir::Files, QDir::Name);
  for (const auto& fileName : fileNames) {
    const auto fileAbsPath = folderNode->filePath().pathAppended(fileName);
    if (workspace_->isKnownSourceFile(fileAbsPath)) {
      continue;  // Skip those belonging to some target.
    }
    // TODO: Handle WORKSPACE files specially: mark as FileType::Project and add a custom icon.
    // List files not belonging to any build target.
    folderNode->addNode(std::make_unique<FileNode>(fileAbsPath, FileType::Unknown));

    // These still need to belong to some RawProjectPart in order for C++ code model to work!
    workspace_->stubPart().files.push_back(fileAbsPath.path());
  }

  // Process subdirectories in the same way.
  const auto& subdirNames = directory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot, QDir::Name);
  for (const auto& subdir : subdirNames) {
    // TODO: Make up a more robust check here.
    if (subdir.startsWith("bazel-")) {
      continue;  // This is one of Bazel's own build dirs. We don't want to go in there.
    }
    auto subdirNode = std::make_unique<FolderNode>(
      Utils::FilePath::fromString(directory.filePath(subdir))
      );
    subdirNode->setDisplayName(subdir);

    buildExplorerFolderContents(subdirNode.get());
    folderNode->addNode(std::move(subdirNode));
  }
}


// --- BazelProject public ---

BazelProject::BazelProject(const Utils::FilePath& fileName)
: Project(Constants::Project::MIMETYPE, fileName),
cppCodeModelUpdater_{std::make_unique<CppEditor::CppProjectUpdater>()}
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
  setBuildSystemCreator([](Target* t) -> BuildSystem* {
    // Yes, the IDE assumes ownership. See `~TargetPrivate` in projectexplorer/target.cpp.
    return new BazelBuildSystem(t);
  });

  startProjectStructureUpdate();
}

// Avoid errors from std::unique_ptr<> around forward declared ProjectScanner (incomplete type).
BazelProject::~BazelProject() = default;

DeploymentKnowledge BazelProject::deploymentKnowledge() const
{
  return DeploymentKnowledge::Bad;
}

// --- BazelProject private ---

QDir BazelProject::workspaceDir() const {
  return QDir(projectDirectory().path());
}

void BazelProject::startProjectStructureUpdate() {
  if (!scannerMutex_.try_lock()) {
    return;
  }
  scanner_->startAsync();
}

void BazelProject::onScanComplete(bool good) {
  std::scoped_lock<std::mutex> lock(std::adopt_lock, scannerMutex_);  // Unlock it no matter what.

  setRootProjectNode(scanner_->takeRootProjectNode());
  bazelWorkspace_ = scanner_->takeWorkspace();

  emit projectScanComplete(good);

  // activeTarget() is null before the user adds one, so skip the C++ code model update if there's
  // none yet.
  if (!activeTarget() || !activeTarget()->activeBuildConfiguration()) {
    return;
  }

  cppCodeModelUpdater_->update(
    ProjectUpdateInfo{
      this,
      KitInfo{activeTarget()->kit()},  // TODO: What if the active Target changes?
      activeTarget()->activeBuildConfiguration()->environment(),
      workspace()->collectProjectParts()
    }
  );
}

}  // namespace BazelProjectManager::Internal


#include "BazelProject.moc"
