#include "globeditor.hpp"

#include "../../model/world/data.hpp"
#include "../../../libs/files/esm/glob.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>

GlobEditor::GlobEditor(Data* data, GlobalVariable* record, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mRecord(record),
      mEditorIdEdit(nullptr),
      mValueSpin(nullptr),
      mFlagsCombo(nullptr)
{
    setupUI();
    loadFromGlob();
}

void GlobEditor::setupUI()
{
    setWindowTitle("Global Variable Editor");
    setMinimumSize(400, 200);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Global Variable Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mValueSpin = new QDoubleSpinBox();
    mValueSpin->setRange(-999999, 999999);
    mValueSpin->setSingleStep(0.01);
    mValueSpin->setDecimals(4);
    infoLayout->addRow("Value:", mValueSpin);

    mFlagsCombo = new QComboBox();
    mFlagsCombo->addItem("None", 0);
    mFlagsCombo->addItem("Constant", 0x40);
    infoLayout->addRow("Flags:", mFlagsCombo);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &GlobEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void GlobEditor::loadFromGlob()
{
    mEditorIdEdit->setText(mRecord->editorId);

    double val = 0.0;
    if (mRecord->value.type == QVariant::Double)
        val = mRecord->value.toDouble();
    else if (mRecord->value.type == QVariant::Int)
        val = static_cast<double>(mRecord->value.toInt());
    else if (mRecord->value.type == QVariant::LongLong)
        val = static_cast<double>(mRecord->value.toLongLong());
    mValueSpin->setValue(val);

    mFlagsCombo->setCurrentIndex(mRecord->constant ? 1 : 0);
}

void GlobEditor::saveRecord()
{
    mRecord->value = mValueSpin->value();
    mRecord->constant = mFlagsCombo->currentIndex() == 1;

    accept();
}
