#pragma once

#include <projectexplorer/project.h>


namespace BazelProjectManager::Internal {

class BazelProject : public ProjectExplorer::Project {
public:
  BazelProject(const Utils::FilePath &fileName);

  // Project interface
  ProjectExplorer::Tasks projectIssues(const ProjectExplorer::Kit* kit) const override;
  ProjectExplorer::ProjectImporter* projectImporter() const override;
  ProjectExplorer::DeploymentKnowledge deploymentKnowledge() const override;

  // TODO: May need to implement this in order to display stuff in the explorer view:
  // ProjectNode *rootProjectNode() const override;

private:
  std::unique_ptr<ProjectExplorer::ProjectImporter> _importer;
};

}  // namespace BazelProjectManager::Internal
