#pragma once

// std
#include <set>

#include <QStandardItemModel>


namespace BazelProjectManager::Internal {

class BazelBuildableItem;
class BazelWorkspace;


/// Data model for the build targets tree view. This class also tracks item selection and provides
/// access to the resulting Bazel build expression.
class BazelBuildTargetsModel : public QStandardItemModel {
  Q_OBJECT
public:
  BazelBuildTargetsModel();

  /// Assign Bazel workspace for display.
  /// @param projectWorkspace - the workspace to display.
  /// @param initialBuildExpressions - a list of active Bazel targets selection.
  void setProjectData(
    const BazelWorkspace* projectWorkspace,
    const QStringList& initialBuildExpressions
  );

  /// @returns a list of Bazel target expressions corresponding to the currently active selection.
  const QStringList buildExpressions() const;

signals:
  /// Emitted when active target selection changes.
  void buildSelectionChanged();

private:
  void onItemChanged(QStandardItem *item);

  std::set<const BazelBuildableItem*> selectedBuildables_;
};

}  // namespace BazelProjectManager::Internal
