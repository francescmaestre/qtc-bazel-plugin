#include "BazelBuildSystem.h"

#include <google/protobuf/util/json_util.h>
#include <projectexplorer/buildconfiguration.h>
#include <utils/filepath.h>

#include "BazelProject.h"
#include "BazelWorkspace.h"
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
  assert(bazelProject());

  // Watch full project parse outcome.
  connect(
    bazelProject(), &BazelProject::projectScanComplete, this, &BazelBuildSystem::onTargetsParsed
  );

  if (!bazelProject()->projectScanned()) {
    // This will eventually invoke `triggerParsing` below.
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
    // TODO: It's probably best to move the implementation here. It was centralized inside
    // BazelProject merely to avoid reparsing for each build configuration but this is not a problem
    // since we're not created per build configuration anymore. In fact, it's likely more correct to
    // have independent parsings per build configurations since some of the bits may differ between
    // those (e.g. paths of output executables).
    bazelProject()->startProjectStructureUpdate();
  }
  catch (const std::exception& e) {
    _parseGuard = {};
    qCWarning(BazelPluginLog) << "Could not start project scan: " << e.what();
  }
  catch (...) {
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

  if (good) {
    Q_ASSERT(bazelProject());
    Q_ASSERT(bazelProject()->workspace());
    // This makes the build targets available for selection to create run configurations.
    setApplicationTargets(
      // Collect runnable targets - filter out DLLs and so on.
      bazelProject()->workspace()->collectBuildTargets(BuildTargetKind::OnlyRunnable)
    );
  }
  else {
    setApplicationTargets({});
  }

  emitBuildSystemUpdated();
}

}  // namespace BazelProjectManager::Internal
