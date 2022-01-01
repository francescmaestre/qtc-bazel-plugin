#pragma once

#include <set>
#include <memory>
#include <mutex>
#include <thread>

#include <projectexplorer/project.h>


namespace CppTools {
class CppProjectUpdater;
}

namespace ProjectExplorer {
class BuildTargetInfo;
}

namespace BazelProjectManager::Internal {

class BazelPackage;

/// This class implements a Bazel project.
/// Populates the project explorer model with info about project sources and structure.
/// Provides information about buildable, runnable, and deployable targets of the project.
class BazelProject final : public ProjectExplorer::Project {
public:
  BazelProject(const Utils::FilePath &fileName);

  /// @returns whether at least one successfull project scan has been complete.
  bool projectScanned() const { return goodScanAtLeastOnce_; }

  /// Begin re-collecting the entire project structure in the background, unless already doing so.
  /// Upon completion this will emit the `projectStructureReady` signal.
  void startProjectStructureUpdate();

  /// @returns a flat list of known build targets as understood by the IDE.
  /// @sa `startProjectStructureUpdate`
  const QList<ProjectExplorer::BuildTargetInfo>& targets() const { return targets_; }

  const std::shared_ptr<const BazelPackage> bazelTargets() const { return bazelPackage_; }

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
  void projectScanComplete(bool good);

protected:
  // Project interface:

  /// Handles the addition of a new target build environment (not a build target, misleading name).
  /// Each target environment may have several build configurations.
  // bool setupTarget(ProjectExplorer::Target* t) override;

private:
  Q_OBJECT

  // BazelBuildSystem should be able to delegate project parsing to a centrally responsible place
  // which is here.
  // friend class BazelBuildSystem;

  QDir workspaceDir() const;

  class ProjectScanner;

  void onScanComplete(bool good);

  std::mutex scannerMutex_;  // Guards the project scanner from multiple invocations.
  std::unique_ptr<ProjectScanner> scanner_;
  bool goodScanAtLeastOnce_ = false;

  // A lightweight collection of buildable targets.
  QList<ProjectExplorer::BuildTargetInfo> targets_;

  // A more detailed targets structure.
  std::shared_ptr<BazelPackage> bazelPackage_;

  std::unique_ptr<CppTools::CppProjectUpdater> cppCodeModelUpdater_;
};

}  // namespace BazelProjectManager::Internal
