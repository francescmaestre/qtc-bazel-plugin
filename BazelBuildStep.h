#pragma once

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/processparameters.h>


namespace Utils
{
class CommandLine;
}

namespace BazelProjectManager::Internal {

/// Registers a "build" step type which invokes Bazel build.
/// This step will become available on the IDE's project setup pane.
class BazelBuildStepFactory final : public ProjectExplorer::BuildStepFactory {
public:
  BazelBuildStepFactory();
};


/// This implements the invokation of Bazel process to perform the build action on some targets.
class BazelBuildStep final : public ProjectExplorer::AbstractProcessStep
{
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

  /// Create UI for extended build step configuration.
  /// @returns the newly created UI widget. Ownership is transferred to the caller!
  /// @see BazelBuildStepConfigWidget
  QWidget* createConfigWidget() override;

  static const char STEP_ID[];

private:
  Q_OBJECT

  /// React to the build args text edit in the config widget being edited.
  void buildArgsEdited(const QString& args);

  /// Update resulting command line after changing the `buildArgs_`.
  void updateCommandLine();

  QString buildFlags_;
  QStringList buildTargets_;
  ProjectExplorer::ProcessParameters params_;  // Holds the resulting command line.
};  // class BazelBuildStep

}  // namespace BazelProjectManager::Internal
