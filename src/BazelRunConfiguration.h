#pragma once

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>


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
  Utils::ProcessRunData runnable() const override;

private:
  /// Updates run configuration from its `BuildTargetInfo`.
  /// Called whenever `RunConfiguration::update` is called.
  void updateTargetInformation();

  /// Prepares an actual launch command line corresponding to the selected build target.
  Utils::CommandLine makeCommandLine();

  /// Aspects are owned by BazelRunConfiguration. When created BaseAspect constructor registers
  /// itself in the AspectContainer without AspectContainer taking ownership automatically.
  /// registerAspect needs to be called explicitly or aspects need to be held as data members
  /// like in this case
  Utils::StringAspect stringAspect_{this};
  ProjectExplorer::ArgumentsAspect argumentAspect_{this};
  ProjectExplorer::TerminalAspect terminalAspect_{this};
};

}  // namespace BazelProjectManager::Internal
