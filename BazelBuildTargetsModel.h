#pragma once

// std
#include <set>

#include <QStandardItemModel>

#include "bazel_helpers.h"


namespace BazelProjectManager::Internal {

/// Map package dir path to set of target names to build.
using BuildSetInfo = std::map<QString, std::set<QString>>;

class BazelBuildableItem;


/// Data model for the build targets tree view. This class allso tracks item selection and provides
/// access to the resulting Bazel build expression.
class BazelBuildTargetsModel : public QStandardItemModel {
  Q_OBJECT
public:
  BazelBuildTargetsModel()
    : QStandardItemModel()
  {
    connect(
      this, &BazelBuildTargetsModel::itemChanged,
      this, &BazelBuildTargetsModel::onItemChanged
    );
  }

  void setProjectData(
    std::shared_ptr<const BazelPackage> projectData,
    const QStringList& initialBuildExpressions
  );

  const QStringList buildExpressions() const;

signals:
  void buildSelectionChanged();

private:
  static
  std::unique_ptr<QStandardItem> buildModelItems(
    std::shared_ptr<const BazelPackage> package,
    const BuildSetInfo& initialBuildSet
  );

  void onItemChanged(QStandardItem *item);

  std::set<const BazelBuildableItem*> selectedBuildables_;
};

}  // namespace BazelProjectManager::Internal
