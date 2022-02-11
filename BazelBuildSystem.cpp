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
   bazelProject(), &BazelProject::projectScanComplete,
   this, &BazelBuildSystem::onTargetsParsed
  );

  if (!bazelProject()->projectScanned()) {
    requestParse();
  }
}

QString BazelBuildSystem::name() const {
  return "BazelBuildSystem";
}

void BazelBuildSystem::triggerParsing() {

  if (_parseGuard.guardsProject())
    return;
  _parseGuard = guardParsingRun();

  try {
    bazelProject()->startProjectStructureUpdate();
  }
  catch(const std::exception& e) {
    _parseGuard = {};
    qCWarning(BazelPluginLog) << "Could not start project scan: " << e.what();
  }
  catch(...) {
    _parseGuard = {};
    qCWarning(BazelPluginLog) << "Could not start project scan for unknown reason.";
  }
}

BazelProject* BazelBuildSystem::bazelProject() const {
  return static_cast<BazelProject*>(project());
}

void BazelBuildSystem::onTargetsParsed(bool good) {
  if (good) {
    _parseGuard.markAsSuccess();  // This is responsible for `emitParsingFinished(true)`.
  }
  _parseGuard = {};

  // This makes the build targets available for selection to create run configurations.
  // TODO: It may make sense to prefilter to only runnable targets - e.g. remove DLLs and so on.
  setApplicationTargets(bazelProject()->targets());
  emitBuildSystemUpdated();
}

}  // namespace BazelProjectManager::Internal
