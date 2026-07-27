#include "misceditor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "../../../libs/files/esm/miscrecord.hpp"
#include "fieldvalidators.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>

MiscEditor::MiscEditor(Data* data, MiscRecord* record, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mRecord(record),
      mEditorIdEdit(nullptr),
      mIconPathEdit(nullptr),
      mModelPathEdit(nullptr),
      mEnchantmentSpin(nullptr),
      mWeightSpin(nullptr),
      mValueSpin(nullptr)
{
    setupUI();
    loadFromMisc();
}

void MiscEditor::setupUI()
{
    setWindowTitle("Misc Item Editor");
    setMinimumSize(400, 300);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Misc Item Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mIconPathEdit = new QLineEdit();
    infoLayout->addRow("Icon Path:", mIconPathEdit);

    mModelPathEdit = new QLineEdit();
    infoLayout->addRow("Model Path:", mModelPathEdit);

    mEnchantmentSpin = new QSpinBox();
    setIntNonNegativeValidator(mEnchantmentSpin);
    infoLayout->addRow("Enchantment:", mEnchantmentSpin);

    mWeightSpin = new QDoubleSpinBox();
    setWeightValidator(mWeightSpin);
    infoLayout->addRow("Weight:", mWeightSpin);

    mValueSpin = new QSpinBox();
    setValueValidator(mValueSpin);
    infoLayout->addRow("Value:", mValueSpin);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &MiscEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void MiscEditor::loadFromMisc()
{
    mEditorIdEdit->setText(mRecord->editorId);
    mIconPathEdit->setText(mRecord->iconPath);
    mModelPathEdit->setText(mRecord->modelPath);
    mWeightSpin->setValue(static_cast<double>(mRecord->weight));
    mValueSpin->setValue(static_cast<int>(mRecord->value));
}

bool MiscEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getMiscCollection().searchId(editorId) >= 0)
    {
        if (editorId != mRecord->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "A misc item with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void MiscEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    {
        auto results = ColumnValidator::validateMisc(*mRecord, mData);
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
    mRecord->weight = static_cast<float>(mWeightSpin->value());
    mRecord->value = static_cast<quint32>(mValueSpin->value());

    accept();
}
