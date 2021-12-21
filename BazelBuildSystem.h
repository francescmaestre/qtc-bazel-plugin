#pragma once

#include <set>

#include <projectexplorer/buildsystem.h>
#include <utils/filepath.h>


namespace BazelProjectManager::Internal {


/// Serves as a bridge between the IDE and the build system.
///
/// Populates the project explorer model with info about project sources and structure.
/// Provides information about buildable, runnable, and deployable targets of the project.
/// Translates back to the underlying build system various refactoring requests: file renaming, etc.
class BazelBuildSystem final : public ProjectExplorer::BuildSystem {
public:
  explicit BazelBuildSystem(ProjectExplorer::BuildConfiguration* buildConfig);

  // BuildSystem interface

  /// Initiate project parsing.
  void triggerParsing() override;

private:
  QDir workspaceDir() const;
  void rebuildProjectStructure(
    ProjectExplorer::FolderNode* folder,
    QList<ProjectExplorer::BuildTargetInfo>& targets,
    std::set<Utils::FilePath>& knownSources
  );

  ProjectExplorer::BuildSystem::ParseGuard _parseGuard;
};

}  // namespace BazelProjectManager::Internal
