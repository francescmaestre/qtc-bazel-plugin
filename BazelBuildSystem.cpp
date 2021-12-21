#include "BazelBuildSystem.h"

#include <google/protobuf/util/json_util.h>
#include <projectexplorer/buildconfiguration.h>
#include <utils/filepath.h>

#include "bazel_helpers.h"
#include "logging.h"


namespace BazelProjectManager::Internal {

namespace {

const char BAZEL_PACKAGE_BUILD_FILE_NAME[] = "BUILD";

/// Create appropriate project nodes out of a Bazel rule item.
///
/// @param bazelRule - rule to process.
/// @param parentFolder [out] - project folder to append new target node to.
/// @param buildTargets [out] - output list to append the build target info to.
/// @param knownSources [out] - output set to insert the target's source paths into.
void processBazelRule(
  const blaze_query::Rule& bazelRule,
  ProjectExplorer::FolderNode* parentFolder,
  QList<ProjectExplorer::BuildTargetInfo>& buildTargets,
  std::set<Utils::FilePath>& knownSources
) {
  // Prepare target description
  {
    ProjectExplorer::BuildTargetInfo targetInfo{};
    targetInfo.buildKey = QString::fromStdString(bazelRule.name());
    targetInfo.displayName = targetInfo.buildKey.split(":").back();  // Un-qualified target name.
    targetInfo.projectFilePath =
      Utils::FilePath::fromString(QString::fromStdString(bazelRule.location()));  // TODO: Strip ':line:column'
    targetInfo.workingDirectory = parentFolder->filePath();
    if (bazelRule.rule_output_size()) {
      const auto& path = bazelRule.rule_output(0);
      targetInfo.targetFilePath = Utils::FilePath::fromUtf8(path.data(), path.size());
    }
    buildTargets.push_back(std::move(targetInfo));
  }

  const ProjectExplorer::BuildTargetInfo& buildTarget = buildTargets.back();

  auto targetNode = std::make_unique<ProjectExplorer::ProjectNode>(
    Utils::FilePath::fromString(buildTarget.displayName)
  );
  // Make this appear differently, not like a normal directory.
  targetNode->setIcon(":/projectexplorer/images/build.png");

  // Collect input sources.
  {
    for (int i = 0; i < bazelRule.rule_input_size(); ++i) {
      const auto& inputLabel = bazelRule.rule_input(i);
      const auto inputLabelQS = QString::fromUtf8(inputLabel.data(), inputLabel.size());
      if (!inputLabelQS.startsWith("//")) {
        continue;  // Ignore external labels or anything that is definitely not a file.
        // FIXME: This may still be a label pointing to another target, not a source file.
        // E.g.:
        // "//lib:hello-time",
        // "//main:hello-greet",
        // "//main:hello-world.cc",
      }
      const auto fileName = inputLabelQS.split(":").back();
      const auto fileAbsPath = parentFolder->filePath().pathAppended(fileName);
      auto fileNode = std::make_unique<ProjectExplorer::FileNode>(
        fileAbsPath,
        ProjectExplorer::FileType::Source
      );
      targetNode->addNode(std::move(fileNode));
      knownSources.insert(fileAbsPath);
    }  // for
  }

  parentFolder->addNode(std::move(targetNode));
}  // processBazelRule

}  // namespace


BazelBuildSystem::BazelBuildSystem(ProjectExplorer::BuildConfiguration* buildConfig)
  : ProjectExplorer::BuildSystem(buildConfig) {

  if (!buildConfig) {
    throw std::runtime_error{"Build configuration missing."};
  }

  // Nobody is gonna `triggerParsing`, we have to initiate this ourselves, duh.
  requestParse();
}

void BazelBuildSystem::triggerParsing() {

  if (_parseGuard.guardsProject())
    return;
  _parseGuard = guardParsingRun();
  qCDebug(BazelPluginLog) << "Reparsing project structure...";
  // TODO: We get here 3 times just by opening a project. Find a way to cache and return earlier.

  QList<ProjectExplorer::BuildTargetInfo> targets;
  std::set<Utils::FilePath> targetSources{projectFilePath()};

  auto rootNode = std::make_unique<ProjectExplorer::ProjectNode>(projectDirectory());
  rootNode->addNode(std::make_unique<ProjectExplorer::FileNode>(
    projectFilePath(),
    ProjectExplorer::FileType::Project
  ));

  rebuildProjectStructure(rootNode.get(), targets, targetSources);

  _parseGuard.markAsSuccess();
  _parseGuard = {};

  setRootProjectNode(std::move(rootNode));
  setApplicationTargets(targets);  // FIXME: This doesn't seem to work.

  emitBuildSystemUpdated();
}

QDir BazelBuildSystem::workspaceDir() const { return projectDirectory().toDir(); }

void BazelBuildSystem::rebuildProjectStructure(
  ProjectExplorer::FolderNode* rootNode,
  QList<ProjectExplorer::BuildTargetInfo>& buildTargets,
  std::set<Utils::FilePath>& knownSources
) {
  QDir rootDir = rootNode->path();

  if (rootDir.exists(BAZEL_PACKAGE_BUILD_FILE_NAME)) {  // This is a Bazel package root.
    // TODO: Add overlay icong to the current folder node.

    const auto fileAbsPath = rootNode->filePath().pathAppended(BAZEL_PACKAGE_BUILD_FILE_NAME);
    // TODO: Add overlay icong to the BUILD file.
    rootNode->addNode(std::make_unique<ProjectExplorer::FileNode>(
      fileAbsPath,
      ProjectExplorer::FileType::Project
    ));
    knownSources.insert(fileAbsPath);

    const auto& packagePath = workspaceDir().relativeFilePath(rootDir.path());
    const auto& [exitCode, qr] = queryPackageRules(workspaceDir().path(), packagePath);

    const auto n_targets = qr.target_size();
    qCDebug(BazelPluginLog)
      << "Bazel package " << packagePath << " has " << n_targets << " targets";

    for (int i = 0; i < n_targets; i++) {
      const auto& bazelTarget = qr.target(i);
      if (bazelTarget.type() != blaze_query::Target_Discriminator_RULE) {
        continue;
      }
      processBazelRule(bazelTarget.rule(), rootNode, buildTargets, knownSources);
    }  // for
  }  // if (rootDir.exists(BAZEL_PACKAGE_BUILD_FILE_NAME))

  // List files not belonging to any build target.
  const auto& fileNames = rootDir.entryList(QDir::Files, QDir::Name);
  for (const auto& fileName : fileNames) {
    const auto fileAbsPath = rootNode->filePath().pathAppended(fileName);
    if (knownSources.find(fileAbsPath) != knownSources.cend()) {
      continue;  // Skip those belonging to some target.
    }
    rootNode->addNode(std::make_unique<ProjectExplorer::FileNode>(
      fileAbsPath,
      ProjectExplorer::FileType::Unknown
    ));
  }

  // Process subdirectories in the same way.
  const auto& subdirNames = rootDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot, QDir::Name);
  for (const auto& subdir : subdirNames) {
    // FIXME: Make up a more robust chek here.
    if (subdir.startsWith("bazel-")) {
      continue;  // This is one of Bazel's own build dirs. We don't want to go in there.
    }
    auto subdirNode = std::make_unique<ProjectExplorer::FolderNode>(
      Utils::FilePath::fromString(rootDir.filePath(subdir))
    );
    subdirNode->setDisplayName(subdir);
    rebuildProjectStructure(subdirNode.get(), buildTargets, knownSources);
    rootNode->addNode(std::move(subdirNode));
  }
}

}  // namespace BazelProjectManager::Internal
