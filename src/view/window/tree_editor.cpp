#include "tree_editor.hpp"

#include "../../model/world/data.hpp"
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
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getTreeCollection().searchId(editorId) >= 0)
    {
        if (editorId != mTree->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "A tree with this Editor ID already exists.");
            return false;
        }
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
