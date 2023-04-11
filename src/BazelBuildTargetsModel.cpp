#include "BazelBuildTargetsModel.h"

// Qt
#include <QIcon>
#include <QStandardItemModel>

// IDE
#include <utils/utilsicons.h>

// Own
#include "bazel_helpers.h"
#include "BazelWorkspace.h"
#include "logging.h"


namespace BazelProjectManager::Internal {

namespace {

static const char ALL_TARGET[] = "all";

}  // namespace

class BazelBuildableItem : public QStandardItem {
public:
  BazelBuildableItem(const QIcon& icon, const QString& text)
    : QStandardItem{icon, text}
  {
    setCheckable(true);
  }

  virtual void updateChildren() {}

  virtual const QString buildExpression() const = 0;
};

namespace {

class BazelTargetItem : public BazelBuildableItem {
public:
  BazelTargetItem(const BuildTarget& target)
    : BazelBuildableItem{Utils::Icons::PROJECT.icon(), target.buildTargetInfo.displayName}
    , target_{target}
  {
    setFlags(flags() | Qt::ItemNeverHasChildren);
  }

  const QString buildExpression() const override {
    return target_.buildTargetInfo.buildKey;
  }

private:
  const BuildTarget& target_;
};


class BazelPackageItem : public BazelBuildableItem {
public:
  BazelPackageItem(std::shared_ptr<const ProjectSubDirectory> package)
    : BazelBuildableItem{Utils::Icons::OPENFILE.icon(), package->name()}
    , package_{package}
  {
    setUserTristate(true);
  }

  const QString buildExpression() const override {
    QString result = package_->bazelPath();
    if (checkState() == Qt::CheckState::Checked) {
      if (!package_->bazelPath().endsWith("/")) {
        result += "/";
      }
      result += "...";
    }
    result += ":";
    result += ALL_TARGET;

    return result;
  }

  void updateChildren() override {
    // NOTE: Whenever updating the children it's important to disable them first - this marks them
    // as not explicitly selected by the user.

    for (int row = 0; row < rowCount(); row++) {
      switch (checkState()) {
        case Qt::CheckState::Checked: {  // Should also check all children recursively.
          auto* const childItem = child(row);
          childItem->setEnabled(false);
          childItem->setCheckState(Qt::CheckState::Checked);
          break;
        }
        case Qt::CheckState::PartiallyChecked: {  // Should also check immediate child targets.
          auto* const childTargetItem = dynamic_cast<BazelTargetItem*>(child(row));
          if (!childTargetItem)
            break;
          childTargetItem->setEnabled(false);
          childTargetItem->setCheckState(Qt::CheckState::Checked);
          break;
        }
        case Qt::CheckState::Unchecked: {  // Enable and uncheck all children.
          auto* const childItem = child(row);
          childItem->setEnabled(true);
          childItem->setCheckState(Qt::CheckState::Unchecked);
          break;
        }
      }  // switch (checkState())
    }  // for
  }

private:
  std::shared_ptr<const ProjectSubDirectory> package_;
};


/// Map package dir path to set of target names to build.
using BuildSetInfo = std::map<QString, std::set<QString>>;


BuildSetInfo parseBuildSet(const QStringList& initialBuildExpressions) {
  BuildSetInfo result;
  for (const auto& labelStr : initialBuildExpressions) {
    // TODO: Conversion from QString to std::string.
    const auto& labelStdStr = labelStr.toStdString();
    const auto maybeParsedLabel = BazelLabel::parse(labelStdStr);
    if (!maybeParsedLabel) {
      qCWarning(BazelPluginLog) << "Can't parse build target expression: '" << labelStr << "'";
      continue;
    }
    // TODO: Conversion from std::string to QString.
    result[QString::fromStdString(std::string{maybeParsedLabel->packageDirPath()})].insert(
      QString::fromStdString(std::string{maybeParsedLabel->targetName()})
    );
  }
  return result;
}

/// Creates a model item corresponding to the given subdirectory, as well as all its children,
/// recursively.
/// @arg subdirectory - project subdirectory to create an item for.
/// @arg initialBuildSet - build set used to mark the items (uncheked/checked/partially)
std::unique_ptr<QStandardItem> buildModelItem(
  std::shared_ptr<const ProjectSubDirectory> subdirectory,
  const BuildSetInfo& initialBuildSet
) {
  const bool dirInBuildSet = [&subdirectory, &initialBuildSet]() {
    for (const auto& [packageDirPath, _] : initialBuildSet) {
      if (subdirectory->isConsumedBy(packageDirPath))
        return true;
    }
    return false;
  }();

  // Figure out which rules from the current package are present in the build set.
  const auto& selectedTargets = [&initialBuildSet, &subdirectory]() -> const std::set<QString>& {
    static const std::set<QString> empty;
    const auto buildSetIter = initialBuildSet.find(subdirectory->dirPath().toString());
    if (buildSetIter == initialBuildSet.end()) {
      return empty;
    }
    return buildSetIter->second;
  }();
  const auto subPackagesBuildSetIter = initialBuildSet.find(subdirectory->dirPath() + "/...");

  bool markAllImmediateChildren = dirInBuildSet;
  bool markAllSubPackages =
    subPackagesBuildSetIter != initialBuildSet.end()
    && subPackagesBuildSetIter->second.count("all");


  // Create current package item.
  auto packageItem = std::make_unique<BazelPackageItem>(subdirectory);

  // TODO: Support alternative syntaxes.
  if (dirInBuildSet || markAllSubPackages || selectedTargets.count("all")) {
    // Disallow changes to items selected implicitly by the parent.
    const bool disableItem = dirInBuildSet && subdirectory->hasParent();
    packageItem->setEnabled(!disableItem);
    packageItem->setCheckState(Qt::CheckState::Checked);
    markAllImmediateChildren = true;
  }

  // Create leaf target items.
  for (const auto& target : subdirectory->targets()) {
    auto targetItem = std::make_unique<BazelTargetItem>(target);

    const auto& targetLabel = target.buildTargetInfo.buildKey;
    if (markAllImmediateChildren || selectedTargets.count(targetLabel)) {
      targetItem->setEnabled(false);
      targetItem->setCheckState(Qt::CheckState::Checked);
    }

    packageItem->appendRow(targetItem.release());
  }

  // Add subpackages recursively.
  for (const auto& [name, subdir] : subdirectory->subDirectories()) {
    packageItem->appendRow(buildModelItem(subdir, initialBuildSet).release());
  }

  return std::move(packageItem);
}

}  // namespace


// PUBLIC

BazelBuildTargetsModel::BazelBuildTargetsModel()
: QStandardItemModel()
{
  connect(
    this, &BazelBuildTargetsModel::itemChanged,
    this, &BazelBuildTargetsModel::onItemChanged
  );
}

void BazelBuildTargetsModel::setProjectData(
const BazelWorkspace* projectWorkspace, const QStringList& initialBuildExpressions
) {
  beginResetModel();
  clear();

  if (projectWorkspace) {
    const auto& initialBuildSet = parseBuildSet(initialBuildExpressions);
    invisibleRootItem()->appendRow(
        buildModelItem(projectWorkspace->rootPackage(), initialBuildSet).release()
    );
    return;
  }
  endResetModel();
}

const QStringList BazelBuildTargetsModel::buildExpressions() const {
  QStringList exprs;
  std::transform(
    selectedBuildables_.cbegin(), selectedBuildables_.cend(),
    std::back_inserter(exprs),
    [](const BazelBuildableItem* item) { return item->buildExpression(); }
  );
  return exprs;
}

// PRIVATE

void BazelBuildTargetsModel::onItemChanged(QStandardItem* item) {
  auto* const buildableItem = dynamic_cast<BazelBuildableItem*>(item);
  if (!buildableItem)
    return;  // Strange but whatever. Maybe log this.

  // Here we rely on the logic inside `updateChildren` which disables the child items before
  // changing their checked state. Thus, here disabled state means the items were selected
  // automatically, for display purpose only, and should not be explicitly included in the build
  // targets set.

  if (!buildableItem->isEnabled() || buildableItem->checkState() == Qt::CheckState::Unchecked) {
    if (selectedBuildables_.erase(buildableItem)) {
      emit buildSelectionChanged();
    }
  }
  else if (buildableItem->checkState() != Qt::CheckState::Unchecked) {
    selectedBuildables_.insert(buildableItem);
    // No matter if the item was already in the set. Since it's check state has changed and it's not
    // unchecked, we consider the selection has changed (e.g. from partial to complete selection).
    emit buildSelectionChanged();
  }

  buildableItem->updateChildren();
}

}  // namespace BazelProjectManager::Internal
