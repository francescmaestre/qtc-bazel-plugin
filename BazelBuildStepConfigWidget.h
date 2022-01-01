#pragma once

#include <memory>

#include <QStringList>
#include <QWidget>

#include "BazelProject.h"


class QStandardItemModel;
class QStandardItem;

namespace Ui {
class BazelBuildStepConfigWidget;
}


namespace BazelProjectManager::Internal {

/// Schematic UI mockup:
///
/// -----------------------------------
/// | Build targets: //...            |
/// | ------------------------------- |
/// | |[ ] MyProject                | |
/// | | l [x] Target 1              | |
/// | | l [_] / Sub-package 1       | |
/// | |    l [x] Target 2           | |
/// | |_____________________________| |
/// |_________________________________|
///
/// The checkboxes control which targets get built:
/// - Target checked: build individually selected targets (`bazel build //<dir>:<target>`)
/// - Dir checked: all rules in all packages underneath (`bazel build //<dir>/...:all`)
///   NOTE: This should automatically check and disable all child items.
/// - Dir "partially" checked: all rules immediately inside (`bazel build //<dir>:all`)
///
class BazelBuildStepConfigWidget : public QWidget
{
  Q_OBJECT

public:
  explicit BazelBuildStepConfigWidget(QWidget *parent = nullptr);
  ~BazelBuildStepConfigWidget();

  void setProjectData(
    std::shared_ptr<const BazelPackage> projectData,
    const QString& buildFlags,
    const QStringList& initialBuildExpressions
  );

  const QStringList buildExpressions() const;

signals:
  void buildFlagsChanged(const QString& flags);
  void buildSelectionChanged();

private:
  QStringList buildTargets_;
  std::unique_ptr<Ui::BazelBuildStepConfigWidget> ui_;
  std::unique_ptr<QStandardItemModel> model_;
};

}  // namespace BazelProjectManager::Internal
