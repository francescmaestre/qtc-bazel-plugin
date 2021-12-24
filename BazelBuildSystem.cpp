#include "BazelBuildSystem.h"

#include <google/protobuf/util/json_util.h>
#include <projectexplorer/buildconfiguration.h>
#include <utils/filepath.h>

#include "BazelProject.h"
#include "bazel_helpers.h"
#include "logging.h"


namespace BazelProjectManager::Internal {


BazelBuildSystem::BazelBuildSystem(ProjectExplorer::Target* target)
  : ProjectExplorer::BuildSystem(target) {
  construct();
}

BazelBuildSystem::BazelBuildSystem(ProjectExplorer::BuildConfiguration* buildConfig)
  : ProjectExplorer::BuildSystem(buildConfig) {
  construct();
}

void BazelBuildSystem::construct() {
  connect(
   bazelProject(), &BazelProject::projectStructureReady,
   this, &BazelBuildSystem::onTargetsParsed
  );

  // In the current implementation by the time we get constructed BazelProject has parsed it all.
  onTargetsParsed();
  // Importand for the IDE. Oherwise the Build button shall stay disabled.
  emitParsingFinished(true);
}

void BazelBuildSystem::triggerParsing() {

  if (_parseGuard.guardsProject())
    return;
  _parseGuard = guardParsingRun();
  qCDebug(BazelPluginLog) << "Reparsing project structure...";

  try {
    bazelProject()->requestReparse();
    _parseGuard.markAsSuccess();  // This is responsible for `emitParsingFinished(true)`.
  }
  catch(const std::exception& e) {
    qCWarning(BazelPluginLog) << "Reparsing project failed: " << e.what();
  }
  catch(...) {
    qCWarning(BazelPluginLog) << "Reparsing project failed with unknown error.";
  }
  _parseGuard = {};
}

BazelProject* BazelBuildSystem::bazelProject() const {
  return static_cast<BazelProject*>(project());
}

void BazelBuildSystem::onTargetsParsed() {
  // FIXME: This should be important but the effect is currently unclear.
  setApplicationTargets(bazelProject()->targets());
  emitBuildSystemUpdated();
}

}  // namespace BazelProjectManager::Internal
