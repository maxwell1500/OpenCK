#include "stat_editor.hpp"

#include "../../model/world/data.hpp"
#include "../../../libs/files/esm/statrecord.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>

StatEditor::StatEditor(Data* data, StatRecord* stat, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mStat(stat),
      mEditorIdEdit(nullptr),
      mModelEdit(nullptr)
{
    setupUI();
    loadFromStat();
}

void StatEditor::setupUI()
{
    setWindowTitle("Static Record Editor");
    setMinimumSize(400, 150);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Static Record Information");
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

    connect(saveBtn, &QPushButton::clicked, this, &StatEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void StatEditor::loadFromStat()
{
    mEditorIdEdit->setText(mStat->editorId);
    mModelEdit->setText(mStat->modelPath);
}

void StatEditor::saveToStat()
{
    mStat->modelPath = mModelEdit->text();
}

bool StatEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getStatCollection().searchId(editorId) >= 0)
    {
        if (editorId != mStat->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "A static with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void StatEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    saveToStat();
    accept();
}
