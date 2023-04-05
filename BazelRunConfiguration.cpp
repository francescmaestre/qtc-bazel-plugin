#include "BazelRunConfiguration.h"

// IDE
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>

// Own
#include "plugin_constants.h"


namespace BazelProjectManager::Internal {
using namespace ProjectExplorer;

const char BazelRunConfiguration::ID[] = "BazelProjectManager.RunConfiguration";


BazelRunConfigurationFactory::BazelRunConfigurationFactory()
  : RunConfigurationFactory() {
  registerRunConfiguration<BazelRunConfiguration>(BazelRunConfiguration::ID);
  addSupportedProjectType(Constants::Project::ID);
  addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);

  // TODO: Check if it makes sense to support Docker::Constants::DOCKER_DEVICE_TYPE
  // (include plugins/docker/dockerconstants.h)
}


BazelRunConfiguration::BazelRunConfiguration(Target* target, Utils::Id id)
  : ProjectExplorer::RunConfiguration(target, id) {

  auto* const envAspect = addAspect<LocalEnvironmentAspect>(target);
  addAspect<WorkingDirectoryAspect>(macroExpander(), envAspect);
  addAspect<ExecutableAspect>(target, ExecutableAspect::RunDevice);
  addAspect<ArgumentsAspect>(macroExpander());
  addAspect<TerminalAspect>();

  setUpdater([this] { updateTargetInformation(); });

  connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
}

void BazelRunConfiguration::updateTargetInformation() {
  if (!activeBuildSystem())
      return;

  const BuildTargetInfo& bti = buildTargetInfo();
  setDefaultDisplayName(bti.displayName);

  aspect<TerminalAspect>()->setUseTerminalHint(bti.usesTerminal);
  aspect<ExecutableAspect>()->setExecutable(bti.targetFilePath);
  aspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(bti.workingDirectory);

  emit aspect<LocalEnvironmentAspect>()->environmentChanged();
}

}  // namespace BazelProjectManager::Internal
