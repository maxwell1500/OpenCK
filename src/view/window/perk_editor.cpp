#include "perk_editor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "perkrecord.hpp"
#include "fieldvalidators.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>

PerkEditor::PerkEditor(Data* data, PerkRecord* perk, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mPerk(perk),
      mEditorIdEdit(nullptr),
      mDescriptionEdit(nullptr),
      mRequirementsEdit(nullptr),
      mIconPathEdit(nullptr),
      mConditionsSpin(nullptr)
{
    setupUI();
    loadFromPerk();
}

void PerkEditor::setupUI()
{
    setWindowTitle("Perk Editor");
    setMinimumSize(400, 350);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Perk Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mDescriptionEdit = new QPlainTextEdit();
    mDescriptionEdit->setMinimumHeight(60);
    infoLayout->addRow("Description:", mDescriptionEdit);

    mRequirementsEdit = new QPlainTextEdit();
    mRequirementsEdit->setMinimumHeight(40);
    infoLayout->addRow("Requirements:", mRequirementsEdit);

    mIconPathEdit = new QLineEdit();
    infoLayout->addRow("Icon Path:", mIconPathEdit);

    mConditionsSpin = new QSpinBox();
    setIntNonNegativeValidator(mConditionsSpin);
    mConditionsSpin->setReadOnly(true);
    infoLayout->addRow("Conditions (read-only):", mConditionsSpin);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &PerkEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void PerkEditor::loadFromPerk()
{
    mEditorIdEdit->setText(mPerk->editorId);
    mDescriptionEdit->setPlainText(mPerk->description);
    mRequirementsEdit->setPlainText(mPerk->requirements);
    mIconPathEdit->setText(mPerk->iconPath);
    mConditionsSpin->setValue(static_cast<int>(mPerk->conditions.size()));
}

void PerkEditor::saveToPerk()
{
    mPerk->editorId = mEditorIdEdit->text();
    mPerk->description = mDescriptionEdit->toPlainText();
    mPerk->requirements = mRequirementsEdit->toPlainText();
    mPerk->iconPath = mIconPathEdit->text();
    Q_UNUSED(mConditionsSpin)
}

bool PerkEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getPerkCollection().searchId(editorId) >= 0)
    {
        if (editorId != mPerk->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "A perk with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void PerkEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    {
        auto results = ColumnValidator::validatePerk(*mPerk, mData);
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

    saveToPerk();
    accept();
}
