#pragma once

#include <set>
#include <thread>
#include <mutex>

#include <projectexplorer/project.h>

namespace ProjectExplorer {
class BuildTargetInfo;
}

namespace BazelProjectManager::Internal {

/// This class implements a Bazel project node in the project explorer.
class BazelProject final : public ProjectExplorer::Project {
public:
  BazelProject(const Utils::FilePath &fileName);

  void requestReparse();
  const QList<ProjectExplorer::BuildTargetInfo>& targets() const { return targets_; }

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
  void projectStructureReady();

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

  /// Begin re-collecting the entire project structure in the background, unless already doing so.
  /// Upon completion this will emit the `projectStructureReady` signal.
  void startProjectStructureUpdate();

  void rebuildProjectStructure(
    ProjectExplorer::FolderNode* rootNode,
    QList<ProjectExplorer::BuildTargetInfo>& buildTargets,
    std::set<Utils::FilePath>& knownSources
  );

  QList<ProjectExplorer::BuildTargetInfo> targets_;
};

}  // namespace BazelProjectManager::Internal
