#include "BazelRunConfiguration.h"

// IDE
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>


namespace BazelProjectManager::Internal {
using namespace ProjectExplorer;

const char BazelRunConfiguration::ID[] = "BazelProjectManager.RunConfiguration";

BazelRunConfiguration::BazelRunConfiguration(Target* target, Utils::Id id)
  : ProjectExplorer::RunConfiguration(target, id) {

  addAspect<LocalEnvironmentAspect>(target);
  addAspect<WorkingDirectoryAspect>();
  addAspect<ExecutableAspect>();
  addAspect<ArgumentsAspect>();
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
