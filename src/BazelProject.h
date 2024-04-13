#pragma once

#include <set>
#include <memory>
#include <mutex>
#include <thread>

#include <QtCore/QFuture>
#include <projectexplorer/project.h>
#include <projectexplorer/rawprojectpart.h>


namespace CppEditor {
class CppProjectUpdater;
}

namespace ProjectExplorer {
class BuildTargetInfo;
}

namespace BazelProjectManager::Internal {

class BazelWorkspace;

/// This class implements a Bazel project in the IDE interface.
/// Populates the project explorer model with info about project sources and structure.
/// Provides information about buildable, runnable, and deployable targets of the project.
class BazelProject final : public ProjectExplorer::Project {
public:
  BazelProject(const Utils::FilePath& fileName);
  ~BazelProject() override;

  /// @returns whether at least one successfull project scan has been complete.
  bool projectScanned() const { return goodScanAtLeastOnce_; }

  /// Begin re-collecting the entire project structure in the background, unless already doing so.
  /// Upon completion this will emit the `projectStructureReady` signal.
  void startProjectStructureUpdate();

  /// Provides access to the Bazel workspace structure.
  /// WARN: This can be null before workspace scanning/parsing is complete.
  /// @see startProjectStructureUpdate
  const BazelWorkspace* workspace() const { return workspace_.get(); }

  // Project interface:

  /// Return an importer which can be used to attempt to discover existing project configuration on
  /// the file system - e.g. in a conventionally named build directory nearby.
  /// Since Bazel manages the build artefacts based on just the BUILD files and the call arguments
  /// there's no need to try to extract anything from its output dirs.
  /// TODO: Think if it makes sense to pick up stuff from the WORKSPACE or .bazelrc files.
  // ProjectExplorer::ProjectImporter* projectImporter() const override;

  /// Tell the IDE how much info we have about the project deployment.
  ProjectExplorer::DeploymentKnowledge deploymentKnowledge() const override;

signals:
  /// Emitted at the end of a project scan.
  /// @param good - true when a scan was done without errors.
  void projectScanComplete(bool good);

protected:
  // Project interface:

  /// Handles the addition of a new target build environment (not a build target, misleading name).
  /// Each target environment may have several build configurations.
  // bool setupTarget(ProjectExplorer::Target* t) override;

private:
  Q_OBJECT

  QDir workspaceDir() const;

  Utils::FilePath workspaceDirPath() const;

  /// Internal scan completion handler.
  void onScanComplete(
    std::unique_ptr<BazelWorkspace> parsedWorkspace,
    std::unique_ptr<ProjectExplorer::ProjectNode> parsedRootProjectNode,
    QSet<Utils::FilePath> buildFilePaths,
    ProjectExplorer::RawProjectPart unknownSourcesPart
  );

  std::mutex scannerMutex_;  // Guards the project scanner from multiple invocations.
  std::optional<QFuture<void>> scanFuture_;  // Kept to control the scanner thread.
  bool goodScanAtLeastOnce_ = false;

  // Contains project structure.
  std::unique_ptr<BazelWorkspace> workspace_;

  // This will collect files not belonging to any Bazel target.
  ProjectExplorer::RawProjectPart unknownSourcesPart_;

  std::unique_ptr<CppEditor::CppProjectUpdater> cppCodeModelUpdater_;
};

}  // namespace BazelProjectManager::Internal
