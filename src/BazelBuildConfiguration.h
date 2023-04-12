#pragma once

#include <memory>

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildconfiguration.h>

#include "bazel_helpers.h"


namespace BazelProjectManager::Internal {

/// Manages build parameters and creates initial build/run steps for a Bazel-bazed project.
class BazelBuildConfiguration final : public ProjectExplorer::BuildConfiguration {
public:
  /// Designated ctor. Called by the IDE to handle a project of the corresponding mime type.
  explicit BazelBuildConfiguration(ProjectExplorer::Target* target, Utils::Id id);

  BazelCompilationMode compileMode() const;

  // BuildConfiguration interface

  /// Interface to the underlying build system.
  /// In our case a "fallback" is provided by the `BazelProject` (see `setBuildSystemCreator(...)`).
  // ProjectExplorer::BuildSystem* buildSystem() const override final;

  /// UI for the project configuration.
  ProjectExplorer::NamedWidget* createConfigWidget() override final;

private:
};


/// This registers BazelBuildConfiguration and associates it with the type appropriate project type.
class BazelBuildConfigurationFactory final : public ProjectExplorer::BuildConfigurationFactory
{
public:
  BazelBuildConfigurationFactory();

private:
  /// Provides info about the supported build modes.
  /// Will be called by the IDE to generate a new build for Bazel projects.
  /// @see ProjectExplorer::BuildConfigurationFactory::BuildGenerator
  QList<ProjectExplorer::BuildInfo> generateBuild(
    const ProjectExplorer::Kit* kit, const Utils::FilePath& projectPath, bool forSetup
  );
};


}  // namespace BazelProjectManager::Internal
