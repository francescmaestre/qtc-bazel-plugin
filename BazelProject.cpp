#include "BazelProject.h"

#include <coreplugin/icontext.h>
#include <cppeditor/cppprojectupdater.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/rawprojectpart.h>
#include <projectexplorer/target.h>
#include <utils/filepath.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include "BazelBuildSystem.h"
#include "bazel_helpers.h"
#include "plugin_constants.h"
#include "logging.h"


namespace BazelProjectManager::Internal {

namespace {
using namespace ProjectExplorer;

// NOTE: WORKSPACE file name is declared in the plugin's JSON manifest file.

const char BAZEL_PACKAGE_BUILD_FILE_NAME[] = "BUILD";
const char BAZEL_PACKAGE_BUILD_FILE_NAME_W_EXT[] = "BUILD.bazel";
const char BUILD_ICON[] = ":/projectexplorer/images/build.png";

/// Contains references to interesting attributes of Bazel rules.
/// WARNING: This struct is NON-OWNING and stores mostly just references!
struct RuleAttributeRefs {
private:
  using StringValueListType = std::remove_reference_t<
    decltype(std::declval<blaze_query::Attribute>().string_list_value())
  >;

public:
  RuleAttributeRefs(const blaze_query::Rule& rule);

  bool is_executable = false;
};

RuleAttributeRefs::RuleAttributeRefs(const blaze_query::Rule& rule) {
  for (int i = 0; i < rule.attribute_size(); i++) {
    const blaze_query::Attribute& attr = rule.attribute(i);

    if (attr.name() == "$is_executable") {
      is_executable = attr.has_boolean_value() && attr.boolean_value();
      continue;
    }
  }  // for
}

}  // namespace


// --- ProjectScanner ---

class BazelProject::ProjectScanner : public QObject {
  Q_OBJECT

public:
  ProjectScanner(Utils::FilePath projectFilePath)
    : projectFilePath_{std::move(projectFilePath)} {
  }

  void startAsync();

  // Since there's only one possible caller of these, we just let take the ownership.

  std::unique_ptr<ProjectNode> takeRootNode() {
    return std::exchange(rootNode_, {});
  }

  std::unique_ptr<BazelPackage> takePackage() {
    return std::exchange(package_, {});
  }

  QList<BuildTargetInfo> takeTargets() {
    return std::exchange(appTargets_, {});
  }

  /// RawProjectParts holds most of a project's C++ code model: inputs, targets, includes.
  RawProjectParts takeParts() {
    return std::exchange(parts_, {});
  }

signals:
  void scanComplete(bool good);

private:
  Utils::FilePath workspaceDirPath() const { return projectFilePath_.parentDir(); }

  QDir workspaceDir() const { return workspaceDirPath().toDir(); }

  RawProjectPart& stubPart() { return parts_[0]; }

  /// Recursively scans filesystem under the given project folder.
  ///
  /// This will collect the information about build targets and the code model as well as populate
  /// the folder with relevant child nodes.
  ///
  /// @param destPackage - container for the discovered targets and sub-packages.
  /// @param folderNode - Project folder corresponding to a real FS directory.
  void scanFolder(FolderNode* folderNode, BazelPackage* destPackage);

  /// Create appropriate project nodes and code model info out of a Bazel rule item.
  ///
  /// @param bazelRule - rule to process.
  /// @param parentFolder [out] - project folder to append new target node to.
  void processBazelRule(
    const blaze_query::Rule& bazelRule,
    FolderNode* parentFolder,
    BazelPackage* destPackage
  );


  Utils::FilePath projectFilePath_;

  std::set<Utils::FilePath> knownSources_;
  std::unique_ptr<ProjectNode> rootNode_;
  std::unique_ptr<BazelPackage> package_;
  QList<BuildTargetInfo> appTargets_;
  RawProjectParts parts_;
};  // class ProjectScanner


void BazelProject::ProjectScanner::startAsync() {
  appTargets_.clear();
  package_ = std::make_unique<BazelPackage>();
  knownSources_.clear();
  // Start off with a part - it will collect files not belonging to any build target. See stubPart.
  parts_ = RawProjectParts{{}};
  rootNode_ = std::make_unique<ProjectNode>(workspaceDirPath());

  Utils::runAsync(
    ProjectExplorerPlugin::sharedThreadPool(),
    [this]() {
      try {
        scanFolder(rootNode_.get(), package_.get());
      }
      catch(const std::exception& e) {
        emit scanComplete(false);
        qCWarning(BazelPluginLog) << "Project scan failed: " << e.what();
      }
      catch(...) {
        emit scanComplete(false);
        qCWarning(BazelPluginLog) << "Project scan failed for unknown reason.";
      }

      emit scanComplete(true);
    }
  );
}

void BazelProject::ProjectScanner::scanFolder(
  FolderNode* folderNode,
  BazelPackage* destPackage
) {
  #warning "FilePath::toDir is marked as deprecated!"
  const QDir& rootDir = folderNode->path().toDir();

  const auto maybeBuildFilePath = [&folderNode]() -> std::optional<Utils::FilePath> {
    auto buildFilePath = folderNode->filePath().pathAppended(BAZEL_PACKAGE_BUILD_FILE_NAME);
    if (buildFilePath.exists())
      return std::move(buildFilePath);
    buildFilePath = folderNode->filePath().pathAppended(BAZEL_PACKAGE_BUILD_FILE_NAME_W_EXT);
    if (buildFilePath.exists())
      return std::move(buildFilePath);
    return std::nullopt;
  }();
  if (maybeBuildFilePath.has_value()) {  // This is a Bazel package root.
    // TODO: Add overlay icong to the folderNode.
    // TODO: Add overlay icong to the BUILD file.
    // Add the BUILD file to the prooject explorer tree.
    folderNode->addNode(std::make_unique<FileNode>(
      *maybeBuildFilePath,
      FileType::Project
    ));
    knownSources_.insert(*maybeBuildFilePath);

    const auto& packageDirPath = workspaceDir().relativeFilePath(rootDir.path());
    const auto& [exitCode, qr] = queryPackageRules(workspaceDirPath().toString(), packageDirPath);

    const auto n_targets = qr.target_size();
    qCDebug(BazelPluginLog)
      << "Bazel package " << packageDirPath << " has " << n_targets << " targets";

    for (int i = 0; i < n_targets; i++) {
      const auto& bazelTarget = qr.target(i);
      if (bazelTarget.type() != blaze_query::Target_Discriminator_RULE) {
        continue;
      }
      processBazelRule(bazelTarget.rule(), folderNode, destPackage);
    }  // for
  }  // if (rootDir.exists(BAZEL_PACKAGE_BUILD_FILE_NAME))

  // List files not belonging to any build target.
  const auto& fileNames = rootDir.entryList(QDir::Files, QDir::Name);
  for (const auto& fileName : fileNames) {
    const auto fileAbsPath = folderNode->filePath().pathAppended(fileName);  // FIXME: Crashing somewhere inside. folderNode rug-pulled?
    if (knownSources_.find(fileAbsPath) != knownSources_.cend()) {
      continue;  // Skip those belonging to some target.
    }
    // TODO: Handle WORKSPACE files specially: mark as FileType::Project and add a custom icon.
    folderNode->addNode(std::make_unique<FileNode>(
      fileAbsPath,
      FileType::Unknown
    ));

    // These still need to belong to some RawProjectPart!
    stubPart().files.push_back(fileAbsPath.path());
  }

  // Process subdirectories in the same way.
  const auto& subdirNames = rootDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot, QDir::Name);
  for (const auto& subdir : subdirNames) {
    // FIXME: Make up a more robust check here.
    if (subdir.startsWith("bazel-")) {
      continue;  // This is one of Bazel's own build dirs. We don't want to go in there.
    }
    auto subdirNode = std::make_unique<FolderNode>(
      Utils::FilePath::fromString(rootDir.filePath(subdir))
    );
    subdirNode->setDisplayName(subdir);
    destPackage->subPackages.push_back(
      std::make_shared<BazelPackage>(
        subdir,
        destPackage,
        BazelPackage::ChildrenContainerType{},
        BazelPackage::TargetsContainerType{}
      )
    );
    scanFolder(subdirNode.get(), destPackage->subPackages.back().get());
    folderNode->addNode(std::move(subdirNode));
  }
}

void BazelProject::ProjectScanner::processBazelRule(
  const blaze_query::Rule& bazelRule,
  FolderNode* parentFolder,
  BazelPackage* destPackage
) {
   const RuleAttributeRefs attrRefs{bazelRule};

  // TODO: This part needs a unit-test!
  // Collect code model info.
  {
    RawProjectPart part;

    const auto& locationComponents = QString::fromStdString(bazelRule.location()).split(":");
    part.setProjectFileLocation(
      locationComponents.at(0),
      locationComponents.size() > 1 ? locationComponents.at(1).toInt() : -1,
      locationComponents.size() > 2 ? locationComponents.at(2).toInt() : -1
    );
    part.buildSystemTarget = QString::fromStdString(bazelRule.name());
    part.displayName = part.buildSystemTarget.split(":").back();  // Un-qualified target name.
    part.buildTargetType = attrRefs.is_executable
      ? ProjectExplorer::BuildTargetType::Executable
      : ProjectExplorer::BuildTargetType::Unknown;  // TODO: Would be nice to distinguish libraries.

    // Bazel's convention is to always export include paths relative to the workspace root.
    part.headerPaths << HeaderPath{workspaceDirPath().toString(), HeaderPathType::User};
    // TODO: part.projectMacros = ...
    // TODO: part.flagsForC = ...
    // TODO: part.flagsForCxx = ...

    // Collect input sources.
    for (int i = 0; i < bazelRule.rule_input_size(); ++i) {
      const auto& inputLabel = bazelRule.rule_input(i);
      auto maybeParsedLabel = BazelLabel::parse(inputLabel);
      if (!maybeParsedLabel) {
        qCWarning(BazelPluginLog) << "Unrecognized target input: " << inputLabel.c_str();
        continue;
      }

      if (maybeParsedLabel->repo().length()) {
        continue;  // TODO: Or can there also be source files from external repos?
      }

      const QString packageDirPath =
        QString::fromStdString(maybeParsedLabel->packageDirPath().str());
      const QString relFilePath = QString::fromStdString(maybeParsedLabel->targetPath().str());

      const Utils::FilePath absFilePath = workspaceDirPath()/packageDirPath/relFilePath;
      if (!absFilePath.exists()) {
        continue;  // This way we filter out inputs which are non-files, or are non-existent.
      }

      knownSources_.insert(absFilePath);
      part.files.push_back(absFilePath.toString());
    }  // for

    parts_.push_back(std::move(part));
  }
  const RawProjectPart& part = parts_.back();

  // Prepare build target description.
  {
    BuildTargetInfo targetInfo{};
    targetInfo.buildKey = part.buildSystemTarget;
    targetInfo.displayName = part.displayName;
    targetInfo.projectFilePath = Utils::FilePath::fromString(part.projectFile);
    // Runnable targets shall be picked up by the IDE and presented in the run menu for selection.
    targetInfo.isQtcRunnable = part.buildTargetType == BuildTargetType::Executable;
    if (bazelRule.rule_output_size()) {
      const auto& outputLabel = bazelRule.rule_output(0);  // We hope this is always the executable.
      auto maybeParsedLabel = BazelLabel::parse(outputLabel);
      if (!maybeParsedLabel) {
        qCWarning(BazelPluginLog) << "Unrecognized target output: " << outputLabel.c_str();
      }
      else {
        // FIXME: This changes depending on the Bazel compilation mode (or, in our terms, the build
        // configuration type) and has to either be updated whenever the IDE switches between build
        // configurations, or the project model has to be kept in multiple instances - again, per
        // build configuration instance.
        targetInfo.targetFilePath =
          workspaceDirPath()
          .pathAppended("bazel-bin")
          .pathAppended(QString::fromStdString(maybeParsedLabel->packageDirPath().str()))
          .pathAppended(QString::fromStdString(maybeParsedLabel->targetPath().str()));
      }
    }
    if (targetInfo.isQtcRunnable && !targetInfo.targetFilePath.isEmpty()) {
      targetInfo.workingDirectory = parentFolder->filePath();
    }

    appTargets_.push_back(std::move(targetInfo));

    destPackage->targets.push_back(part.displayName);
  }
  const BuildTargetInfo& buildTarget = appTargets_.back();

  // Create explorer tree nonde.
  {
    auto targetNode =
      std::make_unique<VirtualFolderNode>(parentFolder->filePath());
    targetNode->setDisplayName(buildTarget.displayName);
    targetNode->setIcon(BUILD_ICON);  // Make this appear differently, not like a normal directory.

    for (const QString& fileAbsPath : part.files) {
      auto fileNode = std::make_unique<FileNode>(
        Utils::FilePath::fromString(fileAbsPath),
        FileType::Source
      );
      // This will add intermediate folder nodes in case file is in a parentFolder's subdirectory.
      targetNode->addNestedNode(std::move(fileNode));
    }  // for

    parentFolder->addNode(std::move(targetNode));
  }
}  // processBazelRule


// --- BazelProject public ---

BazelProject::BazelProject(const Utils::FilePath& fileName)
: Project(Constants::Project::MIMETYPE, fileName),
cppCodeModelUpdater_{std::make_unique<CppEditor::CppProjectUpdater>()}
{
  scanner_ = std::make_unique<ProjectScanner>(projectFilePath());
  connect(scanner_.get(), &ProjectScanner::scanComplete, this, &BazelProject::onScanComplete);

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
  setBuildSystemCreator([](Target* t) -> BuildSystem* {
    // Yes, the IDE assumes ownership. See `~TargetPrivate` in projectexplorer/target.cpp.
    return new BazelBuildSystem(t);
  });

  startProjectStructureUpdate();
}

// Avoid errors from std::unique_ptr<> around forward declared ProjectScanner (incomplete type).
BazelProject::~BazelProject() = default;

DeploymentKnowledge BazelProject::deploymentKnowledge() const
{
  return DeploymentKnowledge::Bad;
}

// --- BazelProject private ---

QDir BazelProject::workspaceDir() const { return projectDirectory().toDir(); }

void BazelProject::startProjectStructureUpdate() {
  if (!scannerMutex_.try_lock()) {
    return;
  }
  scanner_->startAsync();
}

void BazelProject::onScanComplete(bool good) {
  std::scoped_lock<std::mutex> lock(std::adopt_lock, scannerMutex_);  // Unlock it no matter what.

  setRootProjectNode(scanner_->takeRootNode());
  targets_ = scanner_->takeTargets();
  bazelPackage_ = scanner_->takePackage();

  emit projectScanComplete(good);

  if (!activeTarget())
    return;  // activeTarget() is null before adding one.

  // Update C++ code model.
  cppCodeModelUpdater_->update(
    ProjectUpdateInfo{
      this,
      KitInfo{activeTarget()->kit()},  // TODO: What if the active Target changes?
      activeTarget()->activeBuildConfiguration()->environment(),
      scanner_->takeParts()
    }
  );
}

}  // namespace BazelProjectManager::Internal


#include "BazelProject.moc"
