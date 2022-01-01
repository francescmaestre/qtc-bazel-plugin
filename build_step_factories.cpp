#include "build_step_factories.h"

#include <projectexplorer/projectexplorerconstants.h>

#include "BazelBuildStep.h"
#include "BazelCleanStep.h"
#include "plugin_constants.h"


namespace BazelProjectManager::Internal {

BazelBuildStepFactory::BazelBuildStepFactory() {
  registerStep<BazelBuildStep>(BazelBuildStep::STEP_ID);
  setDisplayName(BazelBuildStep::tr("Bazel Build"));
  setSupportedProjectType(Constants::Project::ID);
  setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
}


BazelCleanStepFactory::BazelCleanStepFactory() {
  registerStep<BazelCleanStep>(BazelCleanStep::STEP_ID);
  setDisplayName(BazelCleanStep::tr("Bazel Clean"));
  setSupportedProjectType(Constants::Project::ID);
  setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
}


}  // namespace BazelProjectManager::Internal
