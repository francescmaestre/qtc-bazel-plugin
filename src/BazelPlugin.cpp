#include "BazelPlugin.h"

// Qt Creator API:
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>

// Our stuff:
#include "BazelBuildConfiguration.h"
#include "BazelBuildStep.h"
#include "BazelCleanStep.h"
#include "BazelProject.h"
#include "BazelRunConfiguration.h"
#include "logging.h"


namespace BazelProjectManager::Internal {

/// This is just a collection of components brought in by the plugin which register themselves and
/// hook into various aspects of the IDE.
struct PluginGuts
{
  BazelBuildStepFactory buildStepFactory;
  BazelCleanStepFactory cleanStepFactory;
  BazelBuildConfigurationFactory buildConfigFactory;

  // These 2 enable running the build targets.
  BazelRunConfigurationFactory runConfigurationFactory;
#if (IDE_VERSION_MAJOR < 10)
  ProjectExplorer::RunWorkerFactory runWorkerFactory{
      ProjectExplorer::RunWorkerFactory::make<ProjectExplorer::SimpleTargetRunner>(),
      {ProjectExplorer::Constants::NORMAL_RUN_MODE},
      {
        runConfigurationFactory.runConfigurationId(),
      }
  };
#else  // IDE_VERSION_MAJOR >= 10
  ProjectExplorer::SimpleTargetRunnerFactory runWorkerFactory{
    {
      runConfigurationFactory.runConfigurationId(),
    }
  };
#endif
};


// --- BazelPlugin ---

BazelPlugin::BazelPlugin() = default;

BazelPlugin::~BazelPlugin() = default;

bool BazelPlugin::initialize(const QStringList &arguments, QString *errorString)
{
  Q_UNUSED(arguments)
  Q_UNUSED(errorString)

  qCDebug(BazelPluginLog) << __PRETTY_FUNCTION__;

  ProjectExplorer::ProjectManager::registerProjectType<BazelProject>("text/x-bazel");
  _guts = std::make_unique<PluginGuts>();

  // TODO: Add actions to menus. Connect to other plugins' signals

  // TODO: Add file overlay icons.
  // FileIconProvider::registerIconOverlayForFilename(Constants::Icons::BAZEL, "WORKSPACE.bazel");
  // FileIconProvider::registerIconOverlayForFilename(Constants::Icons::BAZEL, "WORKSPACE");
  // FileIconProvider::registerIconOverlayForFilename(Constants::Icons::BAZEL, "BUILD");

  return true;
}

ExtensionSystem::IPlugin::ShutdownFlag BazelPlugin::aboutToShutdown()
{
  // Save settings
  // Disconnect from signals that are not needed during shutdown
  // Hide UI (if you add UI that is not in the main window directly)
  return SynchronousShutdown;
}

} // namespace BazelProjectManager::Internal
