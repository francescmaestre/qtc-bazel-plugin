#include "BazelProjectImporter.h"

namespace BazelProjectManager::Internal {

BazelProjectImporter::BazelProjectImporter(const Utils::FilePath& path)
  : ProjectExplorer::ProjectImporter(path)
{
}


QStringList BazelProjectImporter::importCandidates() {
  return {};
}

QList<void*> BazelProjectImporter::examineDirectory(
    const Utils::FilePath& importPath, QString* warningMessage) const {
  return {};
}

bool BazelProjectImporter::matchKit(void* directoryData, const ProjectExplorer::Kit* k) const {
  return true;
}

ProjectExplorer::Kit* BazelProjectImporter::createKit(void* directoryData) const {
  return nullptr;
}

const QList<ProjectExplorer::BuildInfo>
BazelProjectImporter::buildInfoList(void* directoryData) const {
  return {};
}

void BazelProjectImporter::deleteDirectoryData(void* directoryData) const {
}

}  // namespace BazelProjectManager::Internal
