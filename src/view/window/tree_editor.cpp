#include "tree_editor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "../../../libs/files/esm/treerecord.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>

TreeEditor::TreeEditor(Data* data, TreeRecord* tree, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mTree(tree),
      mEditorIdEdit(nullptr),
      mModelEdit(nullptr)
{
    setupUI();
    loadFromTree();
}

void TreeEditor::setupUI()
{
    setWindowTitle("Tree Record Editor");
    setMinimumSize(400, 150);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Tree Record Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mModelEdit = new QLineEdit();
    mModelEdit->setPlaceholderText("Path to 3D model...");
    infoLayout->addRow("Model Path:", mModelEdit);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &TreeEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void TreeEditor::loadFromTree()
{
    mEditorIdEdit->setText(mTree->editorId);
    mModelEdit->setText(mTree->modelPath);
}

void TreeEditor::saveToTree()
{
    mTree->modelPath = mModelEdit->text();
}

bool TreeEditor::validate()
{
    const QString editorId = mEditorIdEdit->text().trimmed();

    TreeRecord probe = *mTree;
    probe.editorId = editorId;
    const auto results = ColumnValidator::validateTree(probe, static_cast<Data*>(mData));
    for (const auto& r : results)
    {
        if (r.severity != ColumnValidator::Severity::Error)
            continue;
        // The record's own unchanged editor id is the existing index, not a duplicate.
        if (r.field == QStringLiteral("EditorID") && editorId == mTree->editorId)
            continue;
        QMessageBox::warning(this, "Validation Error", r.message);
        return false;
    }
    return true;
}

void TreeEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    saveToTree();
    accept();
}
