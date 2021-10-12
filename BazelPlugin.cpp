#include "BazelBuildConfiguration.h"
#include "BazelPlugin.h"
#include "BazelProject.h"
#include "plugin_constants.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <projectexplorer/projectmanager.h>

#include <QAction>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>

namespace BazelProjectManager::Internal {

struct PluginGuts
{
  BazelBuildConfigurationFactory buildConfigFactory;
};


BazelPlugin::BazelPlugin()
  : _guts(std::make_unique<PluginGuts>())
{
}

BazelPlugin::~BazelPlugin()
{
  // Unregister objects from the plugin manager's object pool
  // Delete members
}

bool BazelPlugin::initialize(const QStringList &arguments, QString *errorString)
{
  // Register objects in the plugin manager's object pool
  // Load settings
  // Add actions to menus
  // Connect to other plugins' signals
  // In the initialize function, a plugin can be sure that the plugins it
  // depends on have initialized their members.

  Q_UNUSED(arguments)
  Q_UNUSED(errorString)

  ProjectExplorer::ProjectManager::registerProjectType<BazelProject>("text/x-bazel");

  // TODO: Add file overlay icons.
  // FileIconProvider::registerIconOverlayForFilename(Constants::Icons::BAZEL, "WORKSPACE.bazel");
  // FileIconProvider::registerIconOverlayForFilename(Constants::Icons::BAZEL, "WORKSPACE");
  // FileIconProvider::registerIconOverlayForFilename(Constants::Icons::BAZEL, "BUILD");

  return true;
}

void BazelPlugin::extensionsInitialized()
{
  // Retrieve objects from the plugin manager's object pool
  // In the extensionsInitialized function, a plugin can be sure that all
  // plugins that depend on it are completely initialized.
}

ExtensionSystem::IPlugin::ShutdownFlag BazelPlugin::aboutToShutdown()
{
  // Save settings
  // Disconnect from signals that are not needed during shutdown
  // Hide UI (if you add UI that is not in the main window directly)
  return SynchronousShutdown;
}

void BazelPlugin::triggerAction()
{
  QMessageBox::information(Core::ICore::mainWindow(),
  tr("Action Triggered"),
  tr("This is an action from Bazel."));
}

} // namespace BazelProjectManager::Internal
