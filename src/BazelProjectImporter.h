#pragma once

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/projectimporter.h>

namespace BazelProjectManager::Internal {

/// @note THIS IS JUST A STUB AND UNUSED!
///
/// An importer may be used to attempt to discover existing project configuration on the file
/// system - e.g. in a conventionally named build directory nearby.
class BazelProjectImporter final : public ProjectExplorer::ProjectImporter {
public:
  explicit BazelProjectImporter(const Utils::FilePath& path);

  // ProjectImporter interface
public:
  Utils::FilePaths importCandidates() override;

protected:
  QList<void*> examineDirectory(const Utils::FilePath& importPath, QString* warningMessage)
    const override;

  bool matchKit(void* directoryData, const ProjectExplorer::Kit* k) const override;

  ProjectExplorer::Kit* createKit(void* directoryData) const override;

  const QList<ProjectExplorer::BuildInfo> buildInfoList(void* directoryData) const override;

  void deleteDirectoryData(void* directoryData) const override;
};

}  // namespace BazelProjectManager::Internal
