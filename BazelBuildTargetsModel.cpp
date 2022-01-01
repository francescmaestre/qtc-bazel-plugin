#include "BazelBuildTargetsModel.h"

// Qt
#include <QIcon>
#include <QStandardItemModel>

// IDE
#include <utils/utilsicons.h>

// Own
#include "logging.h"


namespace BazelProjectManager::Internal {

namespace {
static const char ALL_TARGET[] = "all";
}  // namespace


class BazelBuildableItem : public QStandardItem {
public:
  BazelBuildableItem(std::shared_ptr<const BazelPackage> package, const QIcon& icon, const QString& text)
    : QStandardItem{icon, text},
      package_{std::move(package)} {
    setCheckable(true);
  }

  const BazelPackage* package() const { return package_.get(); }

  virtual void updateChildren() = 0;

  virtual const QString buildExpression() const = 0;

protected:
  std::shared_ptr<const BazelPackage> package_;
};


class BazelTargetItem : public BazelBuildableItem {
public:
  BazelTargetItem(std::shared_ptr<const BazelPackage> package, const size_t targetIdx)
    : BazelBuildableItem{package, Utils::Icons::PROJECT.icon(), package->targets.at(targetIdx)},
      targetIdx_{targetIdx} {
    setFlags(flags() | Qt::ItemNeverHasChildren);
  }

  // Do nothing - target items have no children.
  virtual void updateChildren() override {}

  const QString buildExpression() const override {
    return package_->bazelPath() + ":" + package_->targets.at(targetIdx_);
  }

private:
  size_t targetIdx_;
};


class BazelPackageItem : public BazelBuildableItem {
public:
  BazelPackageItem(std::shared_ptr<const BazelPackage> package)
    : BazelBuildableItem{package, Utils::Icons::OPENFILE.icon(), package->name} {
    setUserTristate(true);
  }

  const QString buildExpression() const override {
    if (checkState() == Qt::CheckState::Checked) {
      return package_->subPackage("...").targetLabel(ALL_TARGET);
    }
    return package_->targetLabel(ALL_TARGET);
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
};


BuildSetInfo parseBuildSet(const QStringList& initialBuildExpressions) {
  BuildSetInfo result;
  for (const auto& labelStr : initialBuildExpressions) {
    // FIXME: Conversion from QString to std::string.
    const auto& labelStdStr = labelStr.toStdString();
    const auto maybeParsedLabel = BazelLabel::parse(labelStdStr);
    if (!maybeParsedLabel) {
      qCWarning(BazelPluginLog) << "Can't parse build target expression: '" << labelStr << "'";
      continue;
    }
    // FIXME: Conversion from std::string to QString.
    result[QString::fromStdString(maybeParsedLabel->packageDirPath().str())].insert(
      QString::fromStdString(maybeParsedLabel->targetPath().str())
    );
  }
  return result;
}


// static
std::unique_ptr<QStandardItem> BazelBuildTargetsModel::buildModelItems(
  std::shared_ptr<const BazelPackage> package,
  const BuildSetInfo& initialBuildSet
) {
  const bool allChecked = [&package, &initialBuildSet]() {
    for (const auto& [packageDirPath, _] : initialBuildSet) {
      if (package->isConsumedBy(packageDirPath))
        return true;
    }
    return false;
  }();

  // Figure out which rules from the current package are present in the build set.
  const auto& selectedTargets = [&initialBuildSet, &package]() -> const std::set<QString>& {
    static const std::set<QString> empty;
    const auto buildSetIter = initialBuildSet.find(package->dirPath());
    if (buildSetIter == initialBuildSet.end()) {
      return empty;
    }
    return buildSetIter->second;
  }();
  const auto subPackagesBuildSetIter = initialBuildSet.find( package->subPackage("...").dirPath() );

  bool buildAllImmediateChildren = allChecked;
  bool buildAllSubPackages =
    subPackagesBuildSetIter != initialBuildSet.end()
    && subPackagesBuildSetIter->second.count("all");


  // Create current package item.
  auto packageItem = std::make_unique<BazelPackageItem>(package);

  // TODO: Support alternative syntaxes.
  if (allChecked || buildAllSubPackages || selectedTargets.find("all") != selectedTargets.end()) {
    // Disallow changes to items selected implicitly by the parent.
    const bool disableItem = allChecked && package->parentPackage;
    packageItem->setEnabled(!disableItem);
    packageItem->setCheckState(Qt::CheckState::Checked);
    buildAllImmediateChildren = true;
  }

  // Create leaf target items.
  for (size_t i = 0; i < package->targets.size(); i++) {
    auto targetItem = std::make_unique<BazelTargetItem>(package, i);

    if (buildAllImmediateChildren ||
        selectedTargets.find(package->targets.at(i)) != selectedTargets.end()
    ) {
      targetItem->setEnabled(false);
      targetItem->setCheckState(Qt::CheckState::Checked);
    }

    packageItem->appendRow(targetItem.release());
  }

  // Add subpackages recursively.
  for (const auto& subPackage : package->subPackages) {
    packageItem->appendRow(buildModelItems(subPackage, initialBuildSet).release());
  }

  return std::move(packageItem);
}

void BazelBuildTargetsModel::setProjectData(
  std::shared_ptr<const BazelPackage> projectData,
  const QStringList& initialBuildExpressions
) {
  beginResetModel();
  clear();

  if (projectData) {
    const auto& initialBuildSet = parseBuildSet(initialBuildExpressions);
    invisibleRootItem()->appendRow(buildModelItems(projectData, initialBuildSet).release());
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
