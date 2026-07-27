#include "ingr_editor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "../../../libs/files/esm/ingrrecord.hpp"
#include "fieldvalidators.hpp"
#include "logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>

IngrEditor::IngrEditor(Data* data, IngrRecord* ingr, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mIngr(ingr),
      mEditorIdEdit(nullptr),
      mIconPathEdit(nullptr),
      mModelPathEdit(nullptr),
      mEnchantmentSpin(nullptr),
      mWeightSpin(nullptr),
      mValueSpin(nullptr)
{
    setupUI();
    loadFromIngr();
}

void IngrEditor::setupUI()
{
    setWindowTitle("Ingredient Editor");
    setMinimumSize(400, 300);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Ingredient Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mIconPathEdit = new QLineEdit();
    infoLayout->addRow("Icon Path:", mIconPathEdit);

    mModelPathEdit = new QLineEdit();
    infoLayout->addRow("Model Path:", mModelPathEdit);

    infoLayout->addRow("", new QLabel("<b>Stats</b>"));

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

    connect(saveBtn, &QPushButton::clicked, this, &IngrEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void IngrEditor::loadFromIngr()
{
    mEditorIdEdit->setText(mIngr->editorId);
    mIconPathEdit->setText(mIngr->iconPath);
    mModelPathEdit->setText(mIngr->modelPath);
    mWeightSpin->setValue(static_cast<double>(mIngr->weight));
    mValueSpin->setValue(static_cast<int>(mIngr->value));
}

bool IngrEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getIngrCollection().searchId(editorId) >= 0)
    {
        if (editorId != mIngr->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "An ingredient with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void IngrEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    {
        auto results = ColumnValidator::validateIngr(*mIngr, mData);
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

    mIngr->editorId = mEditorIdEdit->text();
    mIngr->iconPath = mIconPathEdit->text();
    mIngr->modelPath = mModelPathEdit->text();
    mIngr->weight = static_cast<float>(mWeightSpin->value());
    mIngr->value = static_cast<quint32>(mValueSpin->value());
    accept();
}
