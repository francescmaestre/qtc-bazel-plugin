#include "BazelRunConfiguration.h"

// IDE
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>
#include <utils/aspects.h>

// Own
#include "BazelBuildConfiguration.h"
#include "plugin_constants.h"
#include "utils/processinterface.h"


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
  stringAspect_.setLabelText(tr("Target:"));
  argumentAspect_.setMacroExpander(macroExpander());

  setUpdater([this] { updateTargetInformation(); });
  setCommandLineGetter(std::bind(&BazelRunConfiguration::makeCommandLine, this));

  connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
}

Utils::ProcessRunData BazelRunConfiguration::runnable() const {
  auto runnable = RunConfiguration::runnable();
  // Bazel has to be run from withing the workspace directory.
  runnable.workingDirectory = project()->rootProjectDirectory();
  return runnable;
}

void BazelRunConfiguration::updateTargetInformation() {
  if (!activeBuildSystem())
    return;

  const BuildTargetInfo& bti = buildTargetInfo();
  setDefaultDisplayName(bti.displayName);

  auto* const targetIdAspect = aspect<Utils::StringAspect>();
  targetIdAspect->setValue(bti.buildKey);
  aspect<TerminalAspect>()->setUseTerminalHint(bti.usesTerminal);
}

Utils::CommandLine BazelRunConfiguration::makeCommandLine() {
  // We delegate launching the target to Bazel. This is required because it needs to set up
  // "run-files" and stuff.
  Utils::CommandLine cmd{"bazel"};
  const BuildTargetInfo bti = buildTargetInfo();
  cmd.addArgs({"run", bti.buildKey});  // `buildKey` is the Bazel binary target id.

  if (auto* proj = project(); proj && proj->activeTarget()) {
    // NOTE: Alt. way to obtain this is via `activeBuildSystem()->buildConfiguration()` but this
    // will yield nullptr since BazelBuildConfiguration is constructed from a Target (see ctor)!
    const auto* const buildConf =
      static_cast<BazelBuildConfiguration*>(proj->activeTarget()->activeBuildConfiguration());
    if (buildConf) {
      cmd.addArgs({"--compilation_mode", compileModeToCLIArg(buildConf->compileMode()).toString()});
    }
  }
  // TODO: Support passing extra arguments to Bazel itself.

  // Passed user-specified arguments to the target.
  cmd.addArgs({"--"});
  cmd.addArgs(aspect<ArgumentsAspect>()->arguments(), Utils::CommandLine::Raw);

  return cmd;
}

}  // namespace BazelProjectManager::Internal
