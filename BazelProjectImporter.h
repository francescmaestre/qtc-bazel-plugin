#pragma once

#include <qtsupport/qtprojectimporter.h>


namespace BazelProjectManager::Internal {

class BazelProjectImporter : public QtSupport::QtProjectImporter
{
public:
  explicit BazelProjectImporter(const Utils::FilePath& path);

  // ProjectImporter interface
public:
  QStringList importCandidates() override;

protected:
  QList<void*> examineDirectory(
    const Utils::FilePath& importPath, QString* warningMessage
  ) const override;

  bool matchKit(void* directoryData, const ProjectExplorer::Kit* k) const override;

  ProjectExplorer::Kit* createKit(void* directoryData) const override;

  const QList<ProjectExplorer::BuildInfo> buildInfoList(void* directoryData) const override;

  void deleteDirectoryData(void* directoryData) const override;
};

}  // namespace BazelProjectManager::Internal
