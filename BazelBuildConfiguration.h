#pragma once

#include <memory>

#include <projectexplorer/buildconfiguration.h>

namespace BazelProjectManager::Internal {

class BazelBuildSystem;


/// Manages build parameters and steps for a Bazel-bazed project.
class BazelBuildConfiguration final : public ProjectExplorer::BuildConfiguration {
public:
  /// Designated ctor. Called by the IDE to handle a project of the corresponding mime type.
  explicit BazelBuildConfiguration(ProjectExplorer::Target* target, Utils::Id id);

  // BuildConfiguration interface

  /// Interface to the underlying build system.
  ProjectExplorer::BuildSystem* buildSystem() const override final;

  /// UI for the project configuration.
  ProjectExplorer::NamedWidget* createConfigWidget() override final;

private:
  std::unique_ptr<BazelBuildSystem> _buildSystem;
};


/// This registers BazelBuildConfiguration as the new build configuration type.
class BazelBuildConfigurationFactory final : public ProjectExplorer::BuildConfigurationFactory
{
public:
    BazelBuildConfigurationFactory();

private:
    /// Provides info about the supported build modes.
    /// @see ProjectExplorer::BuildConfigurationFactory::BuildGenerator
    QList<ProjectExplorer::BuildInfo> generateBuild(
      const ProjectExplorer::Kit* kit, const Utils::FilePath& projectPath, bool forSetup
    );
};


}  // namespace BazelProjectManager::Internal
