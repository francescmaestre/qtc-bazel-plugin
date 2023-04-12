#pragma once

#include <projectexplorer/runconfiguration.h>


namespace BazelProjectManager::Internal {

class BazelRunConfigurationFactory : public ProjectExplorer::RunConfigurationFactory {
public:
  BazelRunConfigurationFactory();
};


/// This is responsible for setting up various "aspects" of runnable targets: executable path,
/// command-line arguments, environment variables, etc. Those aspects shall also be automatically
/// presented in the GUI for project run configurations.
class BazelRunConfiguration : public ProjectExplorer::RunConfiguration {
public:
  static const char ID[];

  /// Once registered, used by the IDE to construct run configuration for one of the build targets.
  BazelRunConfiguration(ProjectExplorer::Target* target, Utils::Id id);

  /// Returns a \l Runnable described by this RunConfiguration.
  ProjectExplorer::Runnable runnable() const override;

private:
  /// Updates run configuration from its `BuildTargetInfo`.
  /// Called whenever `RunConfiguration::update` is called.
  void updateTargetInformation();

  /// Prepares an actual launch command line corresponding to the selected build target.
  Utils::CommandLine makeCommandLine();
};

}  // namespace BazelProjectManager::Internal
