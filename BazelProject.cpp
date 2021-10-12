#include "BazelProject.h"

#include "BazelProjectImporter.h"
#include "plugin_constants.h"

#include <coreplugin/icontext.h>
#include <projectexplorer/projectexplorerconstants.h>


namespace BazelProjectManager::Internal {

BazelProject::BazelProject(const Utils::FilePath& fileName)
  : ProjectExplorer::Project(Constants::Project::MIMETYPE, fileName),
    _importer(std::make_unique<BazelProjectImporter>(fileName)) {

  setId(Constants::Project::ID);

  setProjectLanguages({
    ProjectExplorer::Constants::C_LANGUAGE_ID,
    ProjectExplorer::Constants::CXX_LANGUAGE_ID
  });

  setDisplayName(projectDirectory().fileName());

  setNeedsBuildConfigurations(false);
  setHasMakeInstallEquivalent(false);
  setCanBuildProducts();
}

ProjectExplorer::Tasks BazelProject::projectIssues(const ProjectExplorer::Kit*) const {
  return {};
}

ProjectExplorer::ProjectImporter* BazelProject::projectImporter() const {
  return _importer.get();
}

ProjectExplorer::DeploymentKnowledge BazelProject::deploymentKnowledge() const {
  return ProjectExplorer::DeploymentKnowledge::Bad;
}

}  // namespace BazelProjectManager::Internal
