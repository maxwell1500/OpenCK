#include "race_editor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "Racerecord.hpp"
#include "fieldvalidators.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>

RaceEditor::RaceEditor(Data* data, RaceRecord* race, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mRace(race),
      mEditorIdEdit(nullptr),
      mRaceFlagsSpin(nullptr),
      mNpcVariablesSpin(nullptr),
      mFaceDataSpin(nullptr),
      mHeadDataSpin(nullptr)
{
    setupUI();
    loadFromRace();
}

void RaceEditor::setupUI()
{
    setWindowTitle("Race Editor");
    setMinimumSize(400, 300);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Race Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    infoLayout->addRow("", new QLabel("<b>Stats</b>"));

    mRaceFlagsSpin = new QSpinBox();
    setIntNonNegativeValidator(mRaceFlagsSpin);
    infoLayout->addRow("Race Flags:", mRaceFlagsSpin);

    mNpcVariablesSpin = new QSpinBox();
    setIntNonNegativeValidator(mNpcVariablesSpin);
    infoLayout->addRow("NPC Variables:", mNpcVariablesSpin);

    mFaceDataSpin = new QSpinBox();
    setIntNonNegativeValidator(mFaceDataSpin);
    infoLayout->addRow("Face Data:", mFaceDataSpin);

    mHeadDataSpin = new QSpinBox();
    setIntNonNegativeValidator(mHeadDataSpin);
    infoLayout->addRow("Head Data:", mHeadDataSpin);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &RaceEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void RaceEditor::loadFromRace()
{
    mEditorIdEdit->setText(mRace->editorId);
    mRaceFlagsSpin->setValue(mRace->raceFlags);
    mNpcVariablesSpin->setValue(mRace->npcVariables.isEmpty() ? 0 : mRace->npcVariables.first());
    mFaceDataSpin->setValue(mRace->faceData.isEmpty() ? 0 : mRace->faceData.first());
    mHeadDataSpin->setValue(mRace->headData.isEmpty() ? 0 : mRace->headData.first());
}

bool RaceEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getRaceCollection().searchId(editorId) >= 0)
    {
        if (editorId != mRace->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "A race with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void RaceEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    {
        auto results = ColumnValidator::validateRace(*mRace, mData);
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

    mRace->editorId = mEditorIdEdit->text();
    mRace->raceFlags = mRaceFlagsSpin->value();
    if (!mRace->npcVariables.isEmpty())
        mRace->npcVariables[0] = mNpcVariablesSpin->value();
    else
        mRace->npcVariables.append(mNpcVariablesSpin->value());
    if (!mRace->faceData.isEmpty())
        mRace->faceData[0] = mFaceDataSpin->value();
    else
        mRace->faceData.append(mFaceDataSpin->value());
    if (!mRace->headData.isEmpty())
        mRace->headData[0] = mHeadDataSpin->value();
    else
        mRace->headData.append(mHeadDataSpin->value());

    accept();
}
