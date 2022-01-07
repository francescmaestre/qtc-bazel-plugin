#pragma once

#include <projectexplorer/runconfiguration.h>


namespace BazelProjectManager::Internal {

/// This is responsible for setting up various "aspects" of runnable targets: executable path,
/// command-line arguments, environment variables, etc. Those aspects shall also be automatically
/// presented in the GUI for project run configurations.
class BazelRunConfiguration : public ProjectExplorer::RunConfiguration {
public:
  /// Once registered, used by the IDE to construct run configuration for one of the build targets.
  BazelRunConfiguration(ProjectExplorer::Target* target, Utils::Id id);

  static const char ID[];

private:
  void updateTargetInformation();
};

}  // namespace BazelProjectManager::Internal
