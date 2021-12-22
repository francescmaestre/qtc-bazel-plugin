#include "BazelBuildStep.h"

// Qt
#include <QFormLayout>

// QtCreator:
#include <projectexplorer/target.h>
#include <utils/commandline.h>
#include <utils/filepath.h>

// Own:
#include "BazelBuildSystem.h"
#include "plugin_constants.h"

namespace
{
const char STEP_ID[] = "BazelProjectManager.BuildStep";
const char CONFIG_KEY_TARGETS[] = "BazelProjectManager.BuildStep.Targets";
const char CONFIG_KEY_CMD_ARGS[] = "BazelProjectManager.BuildStep.CmdArgs";

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
  setCommandLineProvider([this] { return params_.command(); });
  setDisplayName(tr("Build Step:", "BazelBuildStep config widget display name."));

  // This should only be done after setting the display name, since the later is used for the step
  // summary which is also set inside.
  updateCommandLine();
}

bool BazelBuildStep::fromMap(const QVariantMap& map)
{
  buildArgs_ = map.value(CONFIG_KEY_CMD_ARGS).toStringList();
  return AbstractProcessStep::fromMap(map);
}

QVariantMap BazelBuildStep::toMap() const
{
  QVariantMap map = AbstractProcessStep::toMap();
  map.insert(CONFIG_KEY_CMD_ARGS, buildArgs_);
  return map;
}

QWidget* BazelBuildStep::createConfigWidget()
{
  auto widget = std::make_unique<QWidget>();

  auto formLayout = new QFormLayout(widget.get());
  formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  formLayout->setContentsMargins(0, 0, 0, 0);

  auto toolArgumentsEdit = std::make_unique<QLineEdit>(widget.get());
  toolArgumentsEdit->setText(Utils::ProcessArgs::joinArgs(buildArgs_));
  QLineEdit* const toolArgumentsEditPtr = toolArgumentsEdit.get();
  formLayout->addRow(tr("Tool arguments:"), toolArgumentsEdit.release());

  connect(
    toolArgumentsEditPtr, &QLineEdit::editingFinished,
    this, [this, toolArgumentsEditPtr]() {
      buildArgsEdited(toolArgumentsEditPtr->text());
    }
  );

  return widget.release();
}

void BazelBuildStep::buildArgsEdited(const QString& args)
{
  buildArgs_ = Utils::ProcessArgs::splitArgs(args);
  updateCommandLine();
}

void BazelBuildStep::updateCommandLine()
{
  Utils::CommandLine cmd{Utils::FilePath::fromString("bazel")};
  cmd.addArg("build");
  cmd.addArgs(buildArgs_);

  params_.setCommandLine(std::move(cmd));
  setupProcessParameters(&params_);

  setSummaryText(params_.summary(displayName()));
}

}  // namespace BazelProjectManager::Internal
