#include "BazelBuildStep.h"

// Qt
#include <QFormLayout>

// QtCreator:
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <utils/commandline.h>
#include <utils/filepath.h>

// Own:
#include "BazelBuildConfiguration.h"
#include "BazelBuildStepConfigWidget.h"
#include "BazelProject.h"
#include "BazelWorkspace.h"
#include "plugin_constants.h"

namespace {
const char CONFIG_KEY_TARGETS[]   = "BazelProjectManager.BuildStep.Targets";
const char CONFIG_KEY_CMD_FLAGS[] = "BazelProjectManager.BuildStep.CmdFlags";

}  // namespace

namespace BazelProjectManager::Internal {

BazelBuildStepFactory::BazelBuildStepFactory() {
  registerStep<BazelBuildStep>(BazelBuildStep::STEP_ID);
  setDisplayName(BazelBuildStep::tr("Bazel Build"));
  setSupportedProjectType(Constants::Project::ID);
  setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
}


// -- BazelBuildStep --

const char BazelBuildStep::STEP_ID[] = "BazelProjectManager.BuildStep";

BazelBuildStep::BazelBuildStep(ProjectExplorer::BuildStepList* bsl, Utils::Id id)
  : ProjectExplorer::AbstractProcessStep(bsl, id) {
  const auto* const buildConfig = static_cast<BazelBuildConfiguration*>(this->buildConfiguration());
  buildFlags_ = "--compilation_mode " + compileModeToCLIArg(buildConfig->compileMode());
  buildTargets_ << "//...:all";  // Build all by default.

  setLowPriority();
  setCommandLineProvider([this] { return params_.command(); });
  setDisplayName(tr("Build Step", "BazelBuildStep config widget display name."));

  // This should only be done after the display name is set, since the later is used for the step
  // summary which is also set inside.
  updateCommandLine();
}

void BazelBuildStep::fromMap(const Utils::Store& map) {
  AbstractProcessStep::fromMap(map);

  buildFlags_   = map.value(CONFIG_KEY_CMD_FLAGS).toString();
  buildTargets_ = map.value(CONFIG_KEY_TARGETS).toStringList();
  updateCommandLine();

  return;
}

void BazelBuildStep::toMap(Utils::Store& map) const {
  AbstractProcessStep::toMap(map);
  map.insert(CONFIG_KEY_CMD_FLAGS, buildFlags_);
  map.insert(CONFIG_KEY_TARGETS, buildTargets_);
}

QWidget* BazelBuildStep::createConfigWidget() {
  const auto* bazelProject = static_cast<BazelProject*>(target()->project());
  assert(bazelProject);

  auto widget = std::make_unique<BazelBuildStepConfigWidget>();
  // At some moments `bazelProject->workspace()` may be null but that's OK.
  widget->setProjectData(bazelProject->workspace(), buildFlags_, buildTargets_);

  connect(
    bazelProject,
    &BazelProject::projectScanComplete,
    widget.get(),
    [this, bazelProject, widget = widget.get()] {
      widget->setProjectData(bazelProject->workspace(), buildFlags_, buildTargets_);
    }
  );

  // Observe UI changes and update the resulting command line.
  connect(
    widget.get(),
    &BazelBuildStepConfigWidget::buildFlagsChanged,
    this,
    [this](const QString& flags) {
      buildFlags_ = flags;
      updateCommandLine();
    }
  );
  connect(
    widget.get(),
    &BazelBuildStepConfigWidget::buildSelectionChanged,
    this,
    [this, widget = widget.get()]() {
      buildTargets_ = widget->buildExpressions();
      updateCommandLine();
    }
  );

  return widget.release();
}

void BazelBuildStep::buildArgsEdited(const QString& args) {
  buildFlags_ = args;
  updateCommandLine();
}

void BazelBuildStep::updateCommandLine() {
  Utils::CommandLine cmd{Utils::FilePath::fromString("bazel")};
  cmd.addArg("build");
  cmd.addArgs(buildFlags_, Utils::CommandLine::Raw);
  cmd.addArgs(buildTargets_);

  params_.setCommandLine(std::move(cmd));
  setupProcessParameters(&params_);

  setSummaryText(params_.summary(displayName()));
}

}  // namespace BazelProjectManager::Internal
