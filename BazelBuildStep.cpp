#include "BazelBuildStep.h"

#include <QLabel>

#include <utils/commandline.h>
#include <utils/filepath.h>
#include <utils/layoutbuilder.h>

#include "plugin_constants.h"

namespace
{
const char STEP_ID[] = "BazelProjectManager.BuildStep";
const char CONFIG_KEY_TARGETS[] = "BazelProjectManager.BuildStep.Targets";

}  // namespace

namespace BazelProjectManager::Internal
{

// -- BazelBuildStepFactory --

BazelBuildStepFactory::BazelBuildStepFactory()
{
  registerStep<BazelBuildStep>(STEP_ID);
  setDisplayName(BazelBuildStep::tr("Bazel Build"));
  setSupportedProjectType(Constants::Project::ID);
}


// -- BazelBuildStep --

BazelBuildStep::BazelBuildStep(ProjectExplorer::BuildStepList* bsl, Utils::Id id)
  : ProjectExplorer::AbstractProcessStep(bsl, id)
{
  setLowPriority();
  setCommandLineProvider([this] { return bazelCommand(); });
}

bool BazelBuildStep::fromMap(const QVariantMap& map)
{
  _targetsList = map.value(CONFIG_KEY_TARGETS).toStringList();
  return AbstractProcessStep::fromMap(map);
}

QVariantMap BazelBuildStep::toMap() const
{
  QVariantMap map = AbstractProcessStep::toMap();
  map.insert(CONFIG_KEY_TARGETS, _targetsList);
  return map;
}

void BazelBuildStep::setupOutputFormatter(Utils::OutputFormatter* formatter)
{
}

QWidget* BazelBuildStep::createConfigWidget()
{
  return nullptr;
}

Utils::CommandLine BazelBuildStep::bazelCommand()
{
  Utils::CommandLine cmd{Utils::FilePath::fromString("bazel")};
  cmd.addArgs({"build", _targetsList});
  return cmd;
}

}  // namespace BazelProjectManager::Internal
