#include "ref_editor.hpp"

#include "../../model/world/data.hpp"
#include "refrecord.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "fieldvalidators.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QMessageBox>

RefEditor::RefEditor(Data* data, RefrRecord* ref, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mRef(ref),
      mEditorIdEdit(nullptr),
      mBaseIdSpin(nullptr),
      mPosXSpin(nullptr),
      mPosYSpin(nullptr),
      mPosZSpin(nullptr),
      mRotXDouble(nullptr),
      mRotYDouble(nullptr),
      mRotZDouble(nullptr),
      mScaleDouble(nullptr),
      mOwnerSpin(nullptr),
      mLockLevelSpin(nullptr),
      mInitiallyDisabledCheck(nullptr)
{
    setupUI();
    loadFromRef();
}

void RefEditor::setupUI()
{
    setWindowTitle(tr("Reference Editor"));
    setMinimumSize(450, 550);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox(tr("Reference Information"));
    auto* infoLayout = new QFormLayout(infoGroup);

    infoLayout->addRow("", new QLabel("<b>" + tr("Reference Properties") + "</b>"));

    mBaseIdSpin = new QSpinBox();
    mBaseIdSpin->setRange(0, 9999);
    infoLayout->addRow(tr("Base ID:"), mBaseIdSpin);

    infoLayout->addRow("", new QLabel("<b>" + tr("Position") + "</b>"));

    mPosXSpin = new QDoubleSpinBox();
    setPositionValidator(mPosXSpin);
    infoLayout->addRow(tr("X:"), mPosXSpin);

    mPosYSpin = new QDoubleSpinBox();
    setPositionValidator(mPosYSpin);
    infoLayout->addRow(tr("Y:"), mPosYSpin);

    mPosZSpin = new QDoubleSpinBox();
    setPositionValidator(mPosZSpin);
    infoLayout->addRow(tr("Z:"), mPosZSpin);

    infoLayout->addRow("", new QLabel("<b>" + tr("Rotation") + "</b>"));

    mRotXDouble = new QDoubleSpinBox();
    setRotationValidator(mRotXDouble);
    infoLayout->addRow(tr("X:"), mRotXDouble);

    mRotYDouble = new QDoubleSpinBox();
    setRotationValidator(mRotYDouble);
    infoLayout->addRow(tr("Y:"), mRotYDouble);

    mRotZDouble = new QDoubleSpinBox();
    setRotationValidator(mRotZDouble);
    infoLayout->addRow(tr("Z:"), mRotZDouble);

    mScaleDouble = new QDoubleSpinBox();
    setScaleValidator(mScaleDouble);
    infoLayout->addRow(tr("Scale:"), mScaleDouble);

    infoLayout->addRow("", new QLabel("<b>" + tr("Properties") + "</b>"));

    mOwnerSpin = new QSpinBox();
    setIntNonNegativeValidator(mOwnerSpin);
    infoLayout->addRow(tr("Owner:"), mOwnerSpin);

    mLockLevelSpin = new QSpinBox();
    setLockLevelValidator(mLockLevelSpin);
    infoLayout->addRow(tr("Lock Level:"), mLockLevelSpin);

    mInitiallyDisabledCheck = new QCheckBox(tr("Initially Disabled"));
    infoLayout->addRow("", mInitiallyDisabledCheck);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton(tr("Save"));
    auto* cancelBtn = new QPushButton(tr("Cancel"));
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &RefEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void RefEditor::loadFromRef()
{
    mBaseIdSpin->setValue(mRef->baseId);
    mPosXSpin->setValue(mRef->posX);
    mPosYSpin->setValue(mRef->posY);
    mPosZSpin->setValue(mRef->posZ);
    mRotXDouble->setValue(mRef->rotX);
    mRotYDouble->setValue(mRef->rotY);
    mRotZDouble->setValue(mRef->rotZ);
    mScaleDouble->setValue(mRef->scale);
    mOwnerSpin->setValue(mRef->owner);
    mLockLevelSpin->setValue(mRef->lockLevel);
    mInitiallyDisabledCheck->setChecked(mRef->initiallyDisabled);
}

bool RefEditor::validate()
{
    return true;
}

void RefEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    auto results = ColumnValidator::validateRef(*mRef, mData);
    QStringList errorMessages;
    for (const auto& r : results)
    {
        if (r.severity == ColumnValidator::Severity::Error)
        {
            errorMessages << QString("%1: %2").arg(r.field, r.message);
        }
    }
    if (!errorMessages.isEmpty())
    {
        QMessageBox::warning(this, tr("Validation Errors"), errorMessages.join("\n"));
        return;
    }

    mRef->baseId = mBaseIdSpin->value();
    mRef->posX = static_cast<float>(mPosXSpin->value());
    mRef->posY = static_cast<float>(mPosYSpin->value());
    mRef->posZ = static_cast<float>(mPosZSpin->value());
    mRef->rotX = static_cast<float>(mRotXDouble->value());
    mRef->rotY = static_cast<float>(mRotYDouble->value());
    mRef->rotZ = static_cast<float>(mRotZDouble->value());
    mRef->scale = static_cast<float>(mScaleDouble->value());
    mRef->owner = mOwnerSpin->value();
    mRef->lockLevel = mLockLevelSpin->value();
    mRef->initiallyDisabled = mInitiallyDisabledCheck->isChecked();

    accept();
}
