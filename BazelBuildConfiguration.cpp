#include "BazelBuildConfiguration.h"

#include <projectexplorer/buildinfo.h>

#include "BazelBuildSystem.h"
#include "plugin_constants.h"

namespace {
const char BUILD_CONFIG_ID[] = "BazelProjectManager.BuildConfiguration";
}

namespace BazelProjectManager::Internal {

BazelBuildConfiguration::BazelBuildConfiguration(ProjectExplorer::Target* target, Utils::Id id)
  : ProjectExplorer::BuildConfiguration(target, id),
    _buildSystem(std::make_unique<BazelBuildSystem>(this)) {
}

ProjectExplorer::BuildSystem* BazelBuildConfiguration::buildSystem() const {
  return _buildSystem.get();
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
  return {};  // FIXME: Implement
}

}  // namespace BazelProjectManager::Internal
