#pragma once

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/processparameters.h>


namespace BazelProjectManager::Internal {

/// Registers a "clean" step type which invokes Bazel cleanup.
/// This step will become available on the IDE's project setup pane.
class BazelCleanStepFactory final : public ProjectExplorer::BuildStepFactory {
public:
  BazelCleanStepFactory();
};


/// This implements the invokation of Bazel process to perform a build action on some target.
class BazelCleanStep final : public ProjectExplorer::AbstractProcessStep
{
public:
  /// A designated ctor. Used by the IDE.
  BazelCleanStep(ProjectExplorer::BuildStepList* bsl, Utils::Id id);

  static const char STEP_ID[];

protected:
  QWidget* createConfigWidget() final;

private:
  Q_OBJECT

  void updateCommandLine();

  ProjectExplorer::ProcessParameters params_;  // Holds the resulting command line.
};  // class BazelCleanStep

}  // namespace BazelProjectManager::Internal
