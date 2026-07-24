#include "genericrecordeditor.hpp"

#include "../../model/world/data.hpp"
#include "../../../libs/files/log/logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>

GenericRecordEditor::GenericRecordEditor(Data* data, BaseRecord* record, const QString& recordTypeName, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mRecord(record),
      mRecordTypeName(recordTypeName),
      mEditorIdEdit(nullptr),
      mFullNameEdit(nullptr),
      mDescEdit(nullptr),
      mLevelSpin(nullptr),
      mWeightSpin(nullptr),
      mValueSpin(nullptr),
      mMarkedCheck(nullptr),
      mVisibleCheck(nullptr),
      mNotesEdit(nullptr)
{
    setupUI();
    loadRecord();
}

void GenericRecordEditor::setupUI()
{
    setWindowTitle(QString("%1 Editor").arg(mRecordTypeName));
    setMinimumSize(500, 400);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Common Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mFullNameEdit = new QLineEdit();
    infoLayout->addRow("Full Name:", mFullNameEdit);

    mDescEdit = new QLineEdit();
    infoLayout->addRow("Description:", mDescEdit);

    mLevelSpin = new QSpinBox();
    mLevelSpin->setRange(0, 9999);
    mLevelSpin->setSuffix("");
    infoLayout->addRow("Level:", mLevelSpin);

    mWeightSpin = new QSpinBox();
    mWeightSpin->setRange(0, 9999);
    mWeightSpin->setSingleStep(10);
    mWeightSpin->setSuffix("");
    infoLayout->addRow("Weight:", mWeightSpin);

    mValueSpin = new QSpinBox();
    mValueSpin->setRange(-999999, 999999);
    mValueSpin->setSingleStep(10);
    infoLayout->addRow("Value:", mValueSpin);

    mMarkedCheck = new QCheckBox("Marked");
    infoLayout->addRow("", mMarkedCheck);

    mVisibleCheck = new QCheckBox("Visible");
    infoLayout->addRow("", mVisibleCheck);

    mainLayout->addWidget(infoGroup);

    auto* notesGroup = new QGroupBox("Notes");
    auto* notesLayout = new QVBoxLayout(notesGroup);
    mNotesEdit = new QTextEdit();
    mNotesEdit->setMaximumHeight(150);
    notesLayout->addWidget(mNotesEdit);
    mainLayout->addWidget(notesGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &GenericRecordEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void GenericRecordEditor::loadRecord()
{
    mEditorIdEdit->setText(mRecord->editorId);
    mFullNameEdit->setText(mRecord->fullName);
    mDescEdit->setText(mRecord->description);
    mLevelSpin->setValue(mRecord->level);
    mWeightSpin->setValue(static_cast<int>(mRecord->weight * 100));
    mValueSpin->setValue(static_cast<int>(mRecord->value));
    mMarkedCheck->setChecked(mRecord->isMarked());
    mVisibleCheck->setChecked(mRecord->isVisible());
    mNotesEdit->setPlainText(mRecord->notes);
}

void GenericRecordEditor::saveRecord()
{
    mRecord->editorId = mEditorIdEdit->text();
    mRecord->fullName = mFullNameEdit->text();
    mRecord->description = mDescEdit->text();
    mRecord->level = mLevelSpin->value();
    mRecord->weight = static_cast<float>(mWeightSpin->value()) / 100.0f;
    mRecord->value = static_cast<float>(mValueSpin->value());
    mRecord->setMarked(mMarkedCheck->isChecked());
    mRecord->setVisible(mVisibleCheck->isChecked());
    mRecord->notes = mNotesEdit->toPlainText();
    
    accept();
}
