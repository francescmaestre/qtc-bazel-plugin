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
#include "plugin_constants.h"
#include "logging.h"


namespace BazelProjectManager::Internal {

namespace {
using namespace ProjectExplorer;

// NOTE: WORKSPACE file name is declared in the plugin's JSON manifest file.

const char BAZEL_WORKSPACE_FILE_NAME[] = "WORKSPACE";
const char BAZEL_PACKAGE_BUILD_FILE_NAME[] = "BUILD";
const char BAZEL_PACKAGE_BUILD_FILE_NAME_W_EXT[] = "BUILD.bazel";


/// Recursively fills the child content under a given project explorer node.
///
/// This will combine the information about build targets with the file system entries and
/// populate the project explorer folder with relevant child nodes.
///
/// @param [in] workspace - Bazel workspace to convert into the project explorer nodes.
/// @param [in] folderNode - project folder corresponding to a real FS directory.
/// @param [out] buildFilePaths - sink for Bazel BUILD file paths located in the project.
/// @param [out] unknownSourcesPart - project part to collect files not belonging to any Bazel target.
/// @param [in] futureInterface - used for cancellation checks.
void buildExplorerFolderContents(
    const BazelWorkspace& workspace,
    FolderNode& folderNode,
    QSet<Utils::FilePath>& buildFilePaths,
    ProjectExplorer::RawProjectPart& unknownSourcesPart,
    QFutureInterface<void>& futureInterface
) {
  if (futureInterface.isCanceled()) {
    return;
  }

  const QDir directory{folderNode.filePath().path()};

  const auto& folderRelativePath =
      folderNode.path().relativeChildPath(workspace.workspaceDirPath());
  const auto packageInThisFolder =
      workspace.rootPackage()->findSubPackage(folderRelativePath.toString());
  if (packageInThisFolder) {
    // TODO: Add overlay icong to the folderNode.
    folderNode.setIcon(ProjectExplorer::DirectoryIcon(Constants::Icons::PACKAGE_OVERLAY_ICON));

    // TODO: Add overlay icong to the BUILD file. Mark accordingly those with `packageContainsErrors`

    // List all build targets of this package (directory).
    for (const auto& target : packageInThisFolder->targets()) {
      auto targetNode = std::make_unique<VirtualFolderNode>(folderNode.filePath());
      targetNode->setDisplayName(target.buildTargetInfo.displayName);
      targetNode->setIcon(Constants::Icons::BUILD_ICON);  // Make it appear differently.

      // List all target's sources and generated files.
      for (const QString& fileAbsPath : target.projectPart.files) {
        auto fileNode = std::make_unique<FileNode>(
          Utils::FilePath::fromString(fileAbsPath),
          FileType::Source
        );
        // This will add intermediate folder nodes in case file is in a folderNode's subdirectory.
        targetNode->addNestedNode(std::move(fileNode));

        // TODO: List generated sources (use ProjectExplorer::Constants::FILEOVERLAY_PRODUCT).
      }  // for

      folderNode.addNode(std::move(targetNode));
    }  // for
  }

  // Check directory contents and list all other files which might be still potentially useful.
  const auto& fileNames = directory.entryList(QDir::Files, QDir::Name);
  for (const auto& fileName : fileNames) {
    const auto fileAbsPath = folderNode.filePath().pathAppended(fileName);
    if (workspace.isKnownSourceFile(fileAbsPath)) {
      continue;  // Skip those belonging to some target.
    }

    // Handle WORKSPACE files specially: mark as FileType::Project and add a custom icon.
    // NOTE: WORKSPACE files are not included into Bazel query output.
    const bool isBazelFile =
        fileName == BAZEL_WORKSPACE_FILE_NAME ||
        fileName == BAZEL_PACKAGE_BUILD_FILE_NAME ||
        fileName == BAZEL_PACKAGE_BUILD_FILE_NAME_W_EXT;
    auto fileNode = std::make_unique<FileNode>(
        fileAbsPath,
        isBazelFile ? FileType::Project : FileType::Unknown
    );
    if (isBazelFile) {
      buildFilePaths.insert(fileAbsPath);
      fileNode->setIcon(QIcon{Constants::Icons::BAZEL_ICON});
    }
    folderNode.addNode(std::move(fileNode));

    // These still need to belong to some RawProjectPart in order for C++ code model to work!
    unknownSourcesPart.files.push_back(fileAbsPath.path());
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

    buildExplorerFolderContents(
        workspace,
        *subdirNode.get(),
        buildFilePaths,
        unknownSourcesPart,
        futureInterface
    );
    folderNode.addNode(std::move(subdirNode));
  }
}

}  // namespace


// --- BazelProject public ---

BazelProject::BazelProject(const Utils::FilePath& fileName)
  : Project(Constants::Project::MIMETYPE, fileName),
    cppCodeModelUpdater_{std::make_unique<CppEditor::CppProjectUpdater>()}
{
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
}

BazelProject::~BazelProject() {
  if (scanFuture_) {
    scanFuture_->cancel();
    scanFuture_->waitForFinished();
  }
}

DeploymentKnowledge BazelProject::deploymentKnowledge() const
{
  return DeploymentKnowledge::Bad;
}

// --- BazelProject private ---

QDir BazelProject::workspaceDir() const {
  return QDir(projectDirectory().path());
}

Utils::FilePath BazelProject::workspaceDirPath() const {
  return projectFilePath().parentDir();
}

void BazelProject::startProjectStructureUpdate() {
  if (!scannerMutex_.try_lock()) {
    return;
  }

  scanFuture_ = Utils::runAsync(
    ProjectExplorerPlugin::sharedThreadPool(),
    [this] (QFutureInterface<void>& futureInterface) -> void {
      try {
        // This will query Bazel for all targets in the workspace and build a tree structure of
        // packages and targets.
        auto workspace = std::make_unique<BazelWorkspace>(workspaceDirPath());
        auto rootProjectNode = std::make_unique<ProjectNode>(workspaceDirPath());
        QSet<Utils::FilePath> buildFilePaths;
        ProjectExplorer::RawProjectPart unknownSourcesPart;

        std::optional<ScopedStopwatchLogger> fileScanWatch{"Files scan duration"};
        buildExplorerFolderContents(
          *workspace,
          *rootProjectNode,
          buildFilePaths,
          unknownSourcesPart,
          futureInterface
        );
        fileScanWatch.reset();

        // Pass the results back into the owner thread.
        QMetaObject::invokeMethod(
          this,
          [
            this,
            ws = std::move(workspace),
            rpn = std::move(rootProjectNode),
            bfs = std::move(buildFilePaths),
            usp = std::move(unknownSourcesPart)
          ] () mutable {
            this->onScanComplete(std::move(ws), std::move(rpn), std::move(bfs), std::move(usp));
          },
          Qt::ConnectionType::QueuedConnection
        );
      }
      catch (const std::exception& e) {
        qCWarning(BazelPluginLog) << "Project scan failed: " << e.what();
      }
      catch (...) {
        qCWarning(BazelPluginLog) << "Project scan failed for unknown reason.";
      }
    }
  );
  // NOTE: DO NOT append `onFailed` handlers to the returned QFuture object.
  // It looks like `runAsync` botches the QFuture somehow, so it crashes.
}

void BazelProject::onScanComplete(
    std::unique_ptr<BazelWorkspace> parsedWorkspace,
    std::unique_ptr<ProjectExplorer::ProjectNode> parsedRootProjectNode,
    QSet<Utils::FilePath> buildFilePaths,
    ProjectExplorer::RawProjectPart unknownSourcesPart
) {
  // This should be finished now, so we don't need to keep hold of it.
  scanFuture_.reset();

  // TODO: Maybe better pass the lock from the scanner thread?
  std::scoped_lock<std::mutex> lock(std::adopt_lock, scannerMutex_);  // Unlock it no matter what.
  setRootProjectNode(std::move(parsedRootProjectNode));
  setExtraProjectFiles(buildFilePaths);
  workspace_ = std::move(parsedWorkspace);
  unknownSourcesPart_ = std::move(unknownSourcesPart);

  emit projectScanComplete(workspace_ && rootProjectNode());

  // activeTarget() is null before the user adds one, so skip the C++ code model update if there's
  // none yet.
  if (!activeTarget() || !activeTarget()->activeBuildConfiguration()) {
    return;
  }

  auto projectParts = workspace()->collectProjectParts();
  projectParts.push_back(unknownSourcesPart_);

  // Actual update is performed on a background thread, this is just triggering it.
  cppCodeModelUpdater_->update(
    ProjectUpdateInfo{
      this,
      KitInfo{activeTarget()->kit()},  // TODO: What if the active Target changes?
      activeTarget()->activeBuildConfiguration()->environment(),
      projectParts
    }
  );
}

}  // namespace BazelProjectManager::Internal


#include "BazelProject.moc"
