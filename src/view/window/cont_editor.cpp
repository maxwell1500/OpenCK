#include "cont_editor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "Contrecord.hpp"
#include "fieldvalidators.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>

ContEditor::ContEditor(Data* data, ContRecord* cont, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mCont(cont),
      mEditorIdEdit(nullptr),
      mIconPathEdit(nullptr),
      mModelPathEdit(nullptr),
      mContentsSpin(nullptr),
      mInventoryControlSpin(nullptr),
      mWeightSpin(nullptr),
      mValueSpin(nullptr)
{
    setupUI();
    loadFromCont();
}

void ContEditor::setupUI()
{
    setWindowTitle("Container Editor");
    setMinimumSize(400, 400);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Container Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mIconPathEdit = new QLineEdit();
    infoLayout->addRow("Icon Path:", mIconPathEdit);

    mModelPathEdit = new QLineEdit();
    infoLayout->addRow("Model Path:", mModelPathEdit);

    infoLayout->addRow("", new QLabel("<b>Stats</b>"));

    mContentsSpin = new QSpinBox();
    setIntNonNegativeValidator(mContentsSpin);
    infoLayout->addRow("Contents:", mContentsSpin);

    mInventoryControlSpin = new QSpinBox();
    setIntNonNegativeValidator(mInventoryControlSpin);
    infoLayout->addRow("Inventory Control:", mInventoryControlSpin);

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

    connect(saveBtn, &QPushButton::clicked, this, &ContEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void ContEditor::loadFromCont()
{
    mEditorIdEdit->setText(mCont->editorId);
    mIconPathEdit->setText(mCont->iconPath);
    mModelPathEdit->setText(mCont->modelPath);
    mContentsSpin->setValue(mCont->contents);
    mInventoryControlSpin->setValue(mCont->inventoryControl);
    mWeightSpin->setValue(mCont->weight);
    mValueSpin->setValue(mCont->value);
}

bool ContEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getContCollection().searchId(editorId) >= 0)
    {
        if (editorId != mCont->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "A container with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void ContEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    {
        auto results = ColumnValidator::validateCont(*mCont, mData);
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

    mCont->editorId = mEditorIdEdit->text();
    mCont->iconPath = mIconPathEdit->text();
    mCont->modelPath = mModelPathEdit->text();
    mCont->contents = mContentsSpin->value();
    mCont->inventoryControl = mInventoryControlSpin->value();
    mCont->weight = static_cast<float>(mWeightSpin->value());
    mCont->value = mValueSpin->value();

    accept();
}
