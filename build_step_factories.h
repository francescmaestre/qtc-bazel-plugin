#pragma once

#include <projectexplorer/buildstep.h>

namespace BazelProjectManager::Internal {

/// Registers a "build" step type which invokes Bazel build.
/// This step will become available on the IDE's project setup pane.
class BazelBuildStepFactory final : public ProjectExplorer::BuildStepFactory {
public:
  BazelBuildStepFactory();
};

/// Registers a "clean" step type which invokes Bazel cleanup.
/// This step will become available on the IDE's project setup pane.
class BazelCleanStepFactory final : public ProjectExplorer::BuildStepFactory {
public:
  BazelCleanStepFactory();
};

}  // namespace BazelProjectManager::Internal
