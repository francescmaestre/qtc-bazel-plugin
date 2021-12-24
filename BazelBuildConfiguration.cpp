#include "BazelBuildConfiguration.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kit.h>
#include <utils/qtcassert.h>

#include "BazelBuildStep.h"
#include "logging.h"
#include "plugin_constants.h"

namespace {
const char BUILD_CONFIG_ID[] = "BazelProjectManager.BuildConfiguration";

enum class CompileMode
{
  Fast,
  Dbg,
  Opt,

  CompileMode_LAST
};

constexpr auto compileModeToInteger(const CompileMode m) {
  return static_cast<std::underlying_type<CompileMode>::type>(m);
}

void operator++(CompileMode& m)  // prefix form
{
  // Yeah, this is unsafe. Stop me! :-D
  m = static_cast<CompileMode>(compileModeToInteger(m) + 1);
}

ProjectExplorer::BuildInfo createBuildInfo(CompileMode mode)
{
  using ProjectExplorer::BuildConfiguration;

  ProjectExplorer::BuildInfo info;
  info.extraInfo = compileModeToInteger(mode);

  switch (mode) {
    case CompileMode::Fast:
      info.typeName = "Fast";
      info.displayName = BuildConfiguration::tr("Fast");
      info.buildType = BuildConfiguration::Unknown;
      break;
    case CompileMode::Dbg:
      info.typeName = "Debug";
      info.displayName = BuildConfiguration::tr("Debug");
      info.buildType = BuildConfiguration::Debug;
      break;
    case CompileMode::Opt:
      info.typeName = "Optimised";
      info.displayName = BuildConfiguration::tr("Optimised");
      info.buildType = BuildConfiguration::Release;
        break;
    default:
        QTC_CHECK(false);
        break;
  }

  return info;
}

}  // namespace

namespace BazelProjectManager::Internal {

BazelBuildConfiguration::BazelBuildConfiguration(ProjectExplorer::Target* target, Utils::Id id)
  : ProjectExplorer::BuildConfiguration(target, id) {
  appendInitialBuildStep(BazelBuildStep::STEP_ID);
}

ProjectExplorer::NamedWidget* BazelBuildConfiguration::createConfigWidget()
{
  // TODO: Maybe provide selectors for commonly used compile flags.
  return nullptr;
}

BazelBuildConfigurationFactory::BazelBuildConfigurationFactory()
{
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
)
{
  Q_UNUSED(projectPath)
  Q_UNUSED(forSetup)

  using ProjectExplorer::BuildInfo;
  QList<BuildInfo> result;

  for (auto mode = CompileMode::Fast; mode != CompileMode::CompileMode_LAST; ++mode) {
      BuildInfo info = createBuildInfo(mode);
      info.factory = this;
      info.kitId = kit->id();

      result << info;
  }

  return result;
}

}  // namespace BazelProjectManager::Internal
