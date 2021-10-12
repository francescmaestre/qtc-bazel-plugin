#pragma once

#include <memory>

#include <projectexplorer/buildconfiguration.h>

namespace BazelProjectManager::Internal {

class BazelBuildSystem;


/// Provides access to the Bazel build system.
class BazelBuildConfiguration final : public ProjectExplorer::BuildConfiguration {
public:
  /// Instantiated by QtC whenever it needs to handle a project of the corresponding mime type.
  explicit BazelBuildConfiguration(ProjectExplorer::Target* target, Utils::Id id);

  // BuildConfiguration interface
  ProjectExplorer::BuildSystem* buildSystem() const override;

private:
  std::unique_ptr<BazelBuildSystem> _buildSystem;
};


/// This registers BazelBuildConfiguration as the new build configuration type.
class BazelBuildConfigurationFactory final : public ProjectExplorer::BuildConfigurationFactory
{
public:
    BazelBuildConfigurationFactory();

private:
    /// @see ProjectExplorer::BuildConfigurationFactory::BuildGenerator
    QList<ProjectExplorer::BuildInfo> generateBuild(
      const ProjectExplorer::Kit* kit, const Utils::FilePath& projectPath, bool forSetup
    );
};


}  // namespace BazelProjectManager::Internal
