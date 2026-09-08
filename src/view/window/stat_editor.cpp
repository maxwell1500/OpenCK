#include "stat_editor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
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
    const QString editorId = mEditorIdEdit->text().trimmed();

    StatRecord probe = *mStat;
    probe.editorId = editorId;
    const auto results = ColumnValidator::validateStat(probe, static_cast<Data*>(mData));
    for (const auto& r : results)
    {
        if (r.severity != ColumnValidator::Severity::Error)
            continue;
        // The record's own unchanged editor id is the existing index, not a duplicate.
        if (r.field == QStringLiteral("EditorID") && editorId == mStat->editorId)
            continue;
        QMessageBox::warning(this, "Validation Error", r.message);
        return false;
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
