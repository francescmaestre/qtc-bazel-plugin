#include "BazelCleanStep.h"

namespace BazelProjectManager::Internal {

const char BazelCleanStep::STEP_ID[] = "BazelProjectManager.CleanStep";

BazelCleanStep::BazelCleanStep(ProjectExplorer::BuildStepList* bsl, Utils::Id id)
  : AbstractProcessStep(bsl, id) {
  setLowPriority();
  setCommandLineProvider([this] { return params_.command(); });
  setDisplayName(tr("Clean Step", "BazelCleanStep config widget display name."));
  updateCommandLine();
}

QWidget* BazelCleanStep::createConfigWidget() {
  return nullptr;
}

void BazelCleanStep::updateCommandLine()
{
  Utils::CommandLine cmd{Utils::FilePath::fromString("bazel")};
  cmd.addArg("clean");

  params_.setCommandLine(std::move(cmd));
  setupProcessParameters(&params_);

  setSummaryText(params_.summary(displayName()));
}

}  // namespace BazelProjectManager::Internal
