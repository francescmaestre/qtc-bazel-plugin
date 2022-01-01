#include "BazelPlugin.h"

// Qt Creator API:
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <projectexplorer/projectmanager.h>

// Our stuff:
#include "BazelBuildConfiguration.h"
#include "BazelProject.h"
#include "build_step_factories.h"
#include "logging.h"
#include "plugin_constants.h"


namespace BazelProjectManager::Internal {

/// This is just a collection of components brought in by the plugin which register themselves and
/// hook into various aspects of the IDE.
struct PluginGuts
{
  BazelBuildStepFactory buildStepFactory;
  BazelCleanStepFactory cleanStepFactory;
  BazelBuildConfigurationFactory buildConfigFactory;
};


BazelPlugin::BazelPlugin() = default;

BazelPlugin::~BazelPlugin() = default;

bool BazelPlugin::initialize(const QStringList &arguments, QString *errorString)
{
  Q_UNUSED(arguments)
  Q_UNUSED(errorString)

  qCDebug(BazelPluginLog) << __PRETTY_FUNCTION__;

  ProjectExplorer::ProjectManager::registerProjectType<BazelProject>("text/x-bazel");
  _guts = std::make_unique<PluginGuts>();

  // TODO:
  // Load settings
  // Add actions to menus
  // Connect to other plugins' signals

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
