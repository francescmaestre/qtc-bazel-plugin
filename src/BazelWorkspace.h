#pragma once

// std
#include <memory>
#include <optional>
#include <vector>

// Qt
#include <QString>
#include <QStringView>
#include <QFileInfo>

// IDE
#include <utils/filepath.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/rawprojectpart.h>

// own
#include <bazelpm_export.h>


namespace BazelProjectManager::Internal {

class ProjectSubDirectory;


enum class BuildTargetKind {
  All,
  OnlyRunnable,
};


/// Represents a project target.
struct BuildTarget {
  ProjectExplorer::RawProjectPart projectPart;
  ProjectExplorer::BuildTargetInfo buildTargetInfo;
};

/// Comparator operator to enable storage in sorted containers.
bool operator<(const BuildTarget& left, const BuildTarget& right);


/// Models a Bazel workspace structure, consisting of a tree of packages.
class BAZELPM_EXPORT BazelWorkspace {
public:
  explicit BazelWorkspace(Utils::FilePath workspaceDirPath);

  const Utils::FilePath& workspaceDirPath() const { return workspaceDirPath_; }

  QDir workspaceDir() const { return QDir(workspaceDirPath().path()); }

  std::shared_ptr<ProjectSubDirectory> rootPackage() const { return rootDir_; }

  bool isKnownSourceFile(const Utils::FilePath& fileAbsPath) const;

  /// Recursively collects information about build targets from all subpackages of the workspace.
  ProjectExplorer::RawProjectParts collectProjectParts() const;

  /// Recursively collects information about build targets from all subpackages of the workspace.
  /// @param kind - kind of targets to collect.
  /// @returns a flat sequence of build targets of the requested kind.
  QVector<ProjectExplorer::BuildTargetInfo> collectBuildTargets(const BuildTargetKind kind) const;

private:
  friend class ProjectSubDirectory;

  /// Called by ProjectSubDirectory to notify of newly added build targets.
  void onBuildTargetAdded(const BuildTarget& target);

  std::set<Utils::FilePath> knownSources_;
  Utils::FilePath workspaceDirPath_;
  std::shared_ptr<ProjectSubDirectory> rootDir_;
};


/// Models a tree of sub-directories and their buildable targets with inputs and outputs.
class BAZELPM_EXPORT ProjectSubDirectory
  : public std::enable_shared_from_this<ProjectSubDirectory> {
public:
  /// Construct a "root" package - i.e. one not really explicitly existing as a Bazel BUILD file.
  explicit ProjectSubDirectory(BazelWorkspace* workspace);

  const QString& name() const;
  BazelWorkspace* workspace() const;
  Utils::FilePath workspaceDirPath() const;

  // NOTE: We want a SORTED container here in order for it to be displayed nicely by default.
  using TargetsContainerType = std::set<BuildTarget>;

  /// Access Bazel targets contained in this directory (package).
  const TargetsContainerType& targets() const { return bazelTargets_; }
  TargetsContainerType& targets() { return bazelTargets_; }

  // NOTE: We want a SORTED container here in order for it to be displayed nicely by default.
  using SubdirectoriesContainerType = std::map<QString, std::shared_ptr<ProjectSubDirectory>>;

  /// Access child directories (Bazel packages) contained in this directory (package).
  const SubdirectoriesContainerType& subDirectories() const { return subDirs_; }

  bool hasParent() const { return parentDir_.use_count(); }

  /// @param path - bazel label (starting with `//`) or a directory path (starting with `/`).
  /// @returns whether this package is covered by the wildcard `path` and is a child of `path`.
  /// E.g. //foo/bar is under //...
  /// But //foo is not under //foo/... - it's the wildcard parent itself.
  bool isConsumedBy(const QStringView path) const;

  /// @returns package directory path, starting with "/".
  QStringView dirPath() const;

  /// @returns Bazel package path, starting with "//".
  const QString& bazelPath() const;

  /// Make `child` an immediate sub-directory of this.
  void addSubDir(std::shared_ptr<ProjectSubDirectory> child);

  /// Creates all necessary sub-directories and returns the innermost.
  /// @returns sub-directory as pointed by the `subDirPath` parameter.
  std::shared_ptr<ProjectSubDirectory> addSubDirectories(const QStringView subDirPath);

  /// @returns a sub-package as found by the given `path`.
  std::shared_ptr<ProjectSubDirectory> findSubPackage(const QStringView path);

  void placeTarget(BuildTarget buildTarget);

private:
  friend class BazelWorkspace;

  explicit ProjectSubDirectory(const QStringView subDirPath);

  BazelWorkspace* workspace_ = nullptr;
  QString name_;
  mutable QString cachedBazelPath_;  // see bazelPath()
  std::weak_ptr<const ProjectSubDirectory> parentDir_;
  SubdirectoriesContainerType subDirs_;
  TargetsContainerType bazelTargets_;
};

}  // namespace BazelProjectManager::Internal
