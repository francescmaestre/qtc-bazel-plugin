#pragma once

#include <memory>

#include <extensionsystem/iplugin.h>


namespace BazelProjectManager::Internal {

struct PluginGuts;

/// Manage Bazel-bazed projects in Qt Creator.
/// Support navigating, building and running targets of Bazel projects.
/// @see On plugin lifetime:
/// http://blog.davidecoppola.com/2019/12/how-to-create-a-qt-creator-plugin/
class BazelPlugin : public ExtensionSystem::IPlugin {
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "BazelProjectManager.json")

public:
  BazelPlugin();
  ~BazelPlugin() override;

  // IPlugin interface:

  /// Here we can be sure that the plugins we depend on have initialized their members.
  bool initialize(const QStringList& arguments, QString* errorString) override;

  ShutdownFlag aboutToShutdown() override;

private:
  std::unique_ptr<PluginGuts> _guts;  // Hides some internals.
};

}  // namespace BazelProjectManager::Internal
