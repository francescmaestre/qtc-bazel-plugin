#include "BazelBuildSystem.h"


namespace BazelProjectManager::Internal {

BazelBuildSystem::BazelBuildSystem(ProjectExplorer::BuildConfiguration* buildConfig)
  : ProjectExplorer::BuildSystem(buildConfig) {
}

void BazelBuildSystem::triggerParsing() {
  // TODO: When later this needs to signal asynchronously, do this:
  // guard = guardParsingRun();
  // ...
  // guard.markAsSuccess();
  // guard = {};

  // For now just let QtC think all is OK.
  emitParsingStarted();
  emitParsingFinished(true);
}

}  // namespace BazelProjectManager::Internal
