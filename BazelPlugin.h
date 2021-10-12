#pragma once

#include <memory>

#include <extensionsystem/iplugin.h>

#include "plugin_global.h"


namespace BazelProjectManager::Internal {

struct PluginGuts;

class BazelPlugin : public ExtensionSystem::IPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "BazelProjectManager.json")

public:
  BazelPlugin();
  ~BazelPlugin() override;

  bool initialize(const QStringList &arguments, QString *errorString) override;
  void extensionsInitialized() override;
  ShutdownFlag aboutToShutdown() override;

private:
  void triggerAction();

  std::unique_ptr<PluginGuts> _guts;  // Hides some internals.
};

} // namespace BazelProjectManager::Internal
