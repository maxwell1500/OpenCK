#include "actieditor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "../../../libs/files/esm/actirecord.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>

ActiEditor::ActiEditor(Data* data, ActiRecord* record, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mRecord(record),
      mEditorIdEdit(nullptr),
      mIconPathEdit(nullptr),
      mModelPathEdit(nullptr),
      mEnchantmentSpin(nullptr)
{
    setupUI();
    loadFromActi();
}

void ActiEditor::setupUI()
{
    setWindowTitle("Activator Editor");
    setMinimumSize(400, 300);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Activator Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mIconPathEdit = new QLineEdit();
    infoLayout->addRow("Icon Path:", mIconPathEdit);

    mModelPathEdit = new QLineEdit();
    infoLayout->addRow("Model Path:", mModelPathEdit);

    mEnchantmentSpin = new QSpinBox();
    mEnchantmentSpin->setRange(0, 9999);
    infoLayout->addRow("Enchantment:", mEnchantmentSpin);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &ActiEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void ActiEditor::loadFromActi()
{
    mEditorIdEdit->setText(mRecord->editorId);
    mIconPathEdit->setText(mRecord->iconPath);
    mModelPathEdit->setText(mRecord->modelPath);
}

bool ActiEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getActiCollection().searchId(editorId) >= 0)
    {
        if (editorId != mRecord->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "An activator with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void ActiEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    {
        auto results = ColumnValidator::validateActi(*mRecord, mData);
        QStringList errorMessages;
        for (const auto& r : results) {
            if (r.severity == ColumnValidator::Severity::Error) {
                errorMessages << QString("%1: %2").arg(r.field, r.message);
            }
        }
        if (!errorMessages.isEmpty()) {
            QMessageBox::warning(this, tr("Validation Errors"), errorMessages.join("\n"));
            return;
        }
    }

    mRecord->iconPath = mIconPathEdit->text();
    mRecord->modelPath = mModelPathEdit->text();

    accept();
}
