#include "BazelBuildStepConfigWidget.h"

// Own
#include "BazelBuildTargetsModel.h"
#include "ui_BazelBuildStepConfigWidget.h"


namespace BazelProjectManager::Internal {

BazelBuildStepConfigWidget::BazelBuildStepConfigWidget(QWidget* parent)
  : QWidget{parent}
  , ui_{std::make_unique<Ui::BazelBuildStepConfigWidget>()}
  , model_{std::make_unique<BazelBuildTargetsModel>()} {
  ui_->setupUi(this);

  connect(ui_->extraArgsLineEdit, &QLineEdit::textChanged, this, [this](const QString& newText) {
    emit buildFlagsChanged(newText);
  });

  auto* const modelPtr = static_cast<BazelBuildTargetsModel*>(model_.get());
  connect(modelPtr, &BazelBuildTargetsModel::buildSelectionChanged, this, [this]() {
    emit buildSelectionChanged();
  });

  ui_->targetsTreeView->setModel(model_.get());
}

void BazelBuildStepConfigWidget::setProjectData(
  const BazelWorkspace* projectWorkspace,
  const QString& buildFlags,
  const QStringList& initialBuildExpressions
) {
  ui_->extraArgsLineEdit->setText(buildFlags);

  auto* const model = static_cast<BazelBuildTargetsModel*>(model_.get());
  model->setProjectData(projectWorkspace, initialBuildExpressions);

  if (!model->invisibleRootItem()->hasChildren())
    return;

  // Expand top-level item for user convenience.
  const auto* const workspaceItem = model->invisibleRootItem()->child(0);
  ui_->targetsTreeView->expand(workspaceItem->index());

  // TODO: Expand items containing user-selected children.
}

const QStringList BazelBuildStepConfigWidget::buildExpressions() const {
  return static_cast<BazelBuildTargetsModel*>(model_.get())->buildExpressions();
}

BazelBuildStepConfigWidget::~BazelBuildStepConfigWidget() = default;

}  // namespace BazelProjectManager::Internal


#include "BazelBuildStepConfigWidget.moc"
