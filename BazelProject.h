#pragma once

#include <projectexplorer/project.h>


namespace BazelProjectManager::Internal {

/// This class implements a Bazel project node in the project explorer.
class BazelProject final : public ProjectExplorer::Project {
public:
  BazelProject(const Utils::FilePath &fileName);

  // Project interface:

  /// Return an importer which can be used to attempt to discover existing project configuration on
  /// the file system - e.g. in a conventionally named build directory nearby.
  /// Since Bazel manages the build artefacts based on just the BUILD files and the call arguments
  /// there's no need to try to extract anything from its output dirs.
  /// TODO: Think if it makes sense to pick up stuff from the WORKSPACE or .bazelrc files.
  // ProjectExplorer::ProjectImporter* projectImporter() const override;

  /// Tell the IDE how much info we have about the project deployment.
  ProjectExplorer::DeploymentKnowledge deploymentKnowledge() const override;

  // TODO: May need to implement this in order to display stuff in the explorer view:
  // ProjectNode *rootProjectNode() const override;
  // Or probably better just setRootProjectNode(...)

protected:
  // Project interface:

  /// Handles the addition of a new target build environment (not a build target, misleading name).
  /// Each target environment may have several build configurations.
  // bool setupTarget(ProjectExplorer::Target* t) override;
};

}  // namespace BazelProjectManager::Internal
