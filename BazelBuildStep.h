#pragma once

#include <projectexplorer/buildstep.h>

#include <projectexplorer/abstractprocessstep.h>


namespace Utils
{
class CommandLine;
}


namespace BazelProjectManager::Internal
{

/// Registers a build step type which invokes Bazel build.
/// This step will become available on the IDE's project setup pane.
class BazelBuildStepFactory final : public ProjectExplorer::BuildStepFactory
{
public:
  BazelBuildStepFactory();
};


/// This implements the invokation of Bazel process to perform a build action on some target.
class BazelBuildStep final : public ProjectExplorer::AbstractProcessStep
{
  Q_OBJECT
public:
  /// A designated ctor. Used by the IDE.
  BazelBuildStep(ProjectExplorer::BuildStepList* bsl, Utils::Id id);

  // ProjectConfiguration interface:

  /// Load from the configuration.
  bool fromMap(const QVariantMap& map) override;

  /// Store configuration.
  QVariantMap toMap() const override;

  // BuildStep interface:

  /// IDEs customization point allowing to process build tool's output in order to detect things
  /// like progress, errors, or paths to project files to turn those into "hyperlinks".
  // void setupOutputFormatter(Utils::OutputFormatter* formatter) override;

  /// Create UI for extended build step configuration. This may provide things like target selection
  /// or invokation options specific to the underlying build tool.
  QWidget* createConfigWidget() override;

private:
  /// Prepares command line to execute bazel for this build step.
  Utils::CommandLine bazelCommand();

  QStringList _targetsList;
};  // class BazelBuildStep


}  // namespace BazelProjectManager::Internal
