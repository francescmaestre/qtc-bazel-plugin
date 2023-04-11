#pragma once

#include <set>

#include <projectexplorer/buildsystem.h>
#include <utils/filepath.h>


namespace BazelProjectManager::Internal {

class BazelProject;

/// Serves as a bridge between the IDE and the build system.
///
/// Translates back to the underlying build system various refactoring requests: file renaming, etc.
/// There are 2 possible ways this can be instantiated:
/// - by each `BuildConfiguration`;
/// - by the builder provided by `Project::setBuildSystemCreator`.
/// In our case the later is preferable since it results in just 1 instance regardless of how many
/// build configurations exist. In future this may change if that's what required to correctly parse
/// projects using conditional targets and such.
class BazelBuildSystem final : public ProjectExplorer::BuildSystem {
public:
  explicit BazelBuildSystem(ProjectExplorer::Target* target);

  explicit BazelBuildSystem(ProjectExplorer::BuildConfiguration* buildConfig);

  // BuildSystem interface

  QString name() const override;

  /// Initiate project parsing.
  ///
  /// Although being part of the public interface, this method doesn't seem to be called by anyone
  /// but ourselves - indirectly through a call to `requestParse()` or similar in the base class.
  /// Still, it's better to keep this functional and doing the right thing.
  void triggerParsing() override;

private:
  Q_OBJECT

  void construct();

  BazelProject* bazelProject() const;

  void onTargetsParsed(bool good);

  ProjectExplorer::BuildSystem::ParseGuard _parseGuard;
};

}  // namespace BazelProjectManager::Internal
