#pragma once

#include <projectexplorer/buildsystem.h>


namespace BazelProjectManager::Internal {

class BazelBuildSystem final : public ProjectExplorer::BuildSystem {
public:
  explicit BazelBuildSystem(ProjectExplorer::BuildConfiguration* buildConfig);

  // BuildSystem interface
  void triggerParsing() override;
};

}  // namespace BazelProjectManager::Internal
