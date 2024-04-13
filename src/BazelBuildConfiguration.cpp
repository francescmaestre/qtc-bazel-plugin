#include "BazelBuildConfiguration.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kit.h>
#include <utils/qtcassert.h>

#include "BazelBuildStep.h"
#include "BazelCleanStep.h"
#include "logging.h"
#include "plugin_constants.h"


namespace BazelProjectManager::Internal {

namespace {
const char BUILD_CONFIG_ID[] = "BazelProjectManager.BuildConfiguration";

constexpr auto compileModeToInteger(const BazelCompilationMode m) {
  return static_cast<std::underlying_type<BazelCompilationMode>::type>(m);
}

void operator++(BazelCompilationMode& m)  // prefix form
{
  // Yeah, this is unsafe. Stop me! :-D
  m = static_cast<BazelCompilationMode>(compileModeToInteger(m) + 1);
}

ProjectExplorer::BuildInfo createBuildInfo(BazelCompilationMode mode) {
  using ProjectExplorer::BuildConfiguration;

  ProjectExplorer::BuildInfo info;
  info.extraInfo = compileModeToInteger(mode);

  switch (mode) {
    case BazelCompilationMode::Fast:
      info.typeName    = "Fast";
      info.displayName = BuildConfiguration::tr("Fast");
      info.buildType   = BuildConfiguration::Unknown;
      break;
    case BazelCompilationMode::Dbg:
      info.typeName    = "Debug";
      info.displayName = BuildConfiguration::tr("Debug");
      info.buildType   = BuildConfiguration::Debug;
      break;
    case BazelCompilationMode::Opt:
      info.typeName    = "Optimised";
      info.displayName = BuildConfiguration::tr("Optimised");
      info.buildType   = BuildConfiguration::Release;
      break;
    default: QTC_CHECK(false); break;
  }

  return info;
}

}  // namespace

BazelBuildConfiguration::BazelBuildConfiguration(ProjectExplorer::Target* target, Utils::Id id)
  : ProjectExplorer::BuildConfiguration(target, id) {
  appendInitialBuildStep(BazelBuildStep::STEP_ID);
  appendInitialCleanStep(BazelCleanStep::STEP_ID);
}

BazelCompilationMode BazelBuildConfiguration::compileMode() const {
  // Build steps are created BEFORE build configuration's initializer is run. So guessing from
  // `buildType()` is the only way to provide this info to our build steps as they get created.

  switch (buildType()) {
    case ProjectExplorer::BuildConfiguration::Unknown: return BazelCompilationMode::Fast;
    case ProjectExplorer::BuildConfiguration::Debug: return BazelCompilationMode::Dbg;
    case ProjectExplorer::BuildConfiguration::Release: return BazelCompilationMode::Opt;
  }
  return BazelCompilationMode::Fast;
}

ProjectExplorer::NamedWidget* BazelBuildConfiguration::createConfigWidget() {
  // TODO: Maybe provide selectors for commonly used compile flags.
  return nullptr;
}

BazelBuildConfigurationFactory::BazelBuildConfigurationFactory() {
  registerBuildConfiguration<BazelBuildConfiguration>(BUILD_CONFIG_ID);
  setSupportedProjectType(Constants::Project::ID);
  setSupportedProjectMimeTypeName(Constants::Project::MIMETYPE);
  setBuildGenerator(
    [this](const ProjectExplorer::Kit* kit, const Utils::FilePath& projectPath, bool forSetup) {
      return this->generateBuild(kit, projectPath, forSetup);
    }
  );
}

QList<ProjectExplorer::BuildInfo> BazelBuildConfigurationFactory::generateBuild(
  const ProjectExplorer::Kit* kit, const Utils::FilePath& projectPath, bool forSetup
) {
  Q_UNUSED(projectPath)
  Q_UNUSED(forSetup)

  using ProjectExplorer::BuildInfo;
  QList<BuildInfo> result;

  for (auto mode = BazelCompilationMode::Fast; mode != BazelCompilationMode::CompileMode_LAST;
       ++mode) {
    BuildInfo info = createBuildInfo(mode);
    info.factory   = this;
    info.kitId     = kit->id();

    result << info;
  }

  return result;
}

}  // namespace BazelProjectManager::Internal
