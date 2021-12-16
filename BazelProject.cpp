#include "BazelProject.h"

#include "BazelProjectImporter.h"
#include "plugin_constants.h"

#include <coreplugin/icontext.h>
#include <projectexplorer/projectexplorerconstants.h>


namespace BazelProjectManager::Internal {

BazelProject::BazelProject(const Utils::FilePath& fileName)
  : ProjectExplorer::Project(Constants::Project::MIMETYPE, fileName)
{
  setId(Constants::Project::ID);
  setDisplayName(projectDirectory().fileName());

  setProjectLanguages({
    ProjectExplorer::Constants::C_LANGUAGE_ID,
    ProjectExplorer::Constants::CXX_LANGUAGE_ID
  });

  setNeedsBuildConfigurations(true);
  setNeedsDeployConfigurations(false);
  setHasMakeInstallEquivalent(false);
  setCanBuildProducts();
}

ProjectExplorer::DeploymentKnowledge BazelProject::deploymentKnowledge() const {
  return ProjectExplorer::DeploymentKnowledge::Bad;
}

}  // namespace BazelProjectManager::Internal
