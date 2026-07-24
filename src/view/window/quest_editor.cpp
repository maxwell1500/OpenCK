#include "quest_editor.hpp"
#include "queststageeditor.hpp"
#include "questaliaseditor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "Questrecord.hpp"
#include "logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>

QuestEditor::QuestEditor(Data* data, QuestRecord* quest, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mQuest(quest),
      mEditorIdEdit(nullptr),
      mQuestNameEdit(nullptr),
      mQuestTypeEdit(nullptr),
      mQuestDescEdit(nullptr),
      mDialogueViewEdit(nullptr),
      mStartRankSpin(nullptr),
      mStagesBtn(nullptr),
      mAliasesBtn(nullptr)
{
    setupUI();
    loadFromQuest();
}

void QuestEditor::setupUI()
{
    setWindowTitle("Quest Editor");
    setMinimumSize(500, 400);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Quest Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mQuestNameEdit = new QLineEdit();
    infoLayout->addRow("Quest Name:", mQuestNameEdit);

    mQuestTypeEdit = new QLineEdit();
    infoLayout->addRow("Quest Type:", mQuestTypeEdit);

    mStartRankSpin = new QSpinBox();
    mStartRankSpin->setRange(0, 9999);
    infoLayout->addRow("Start Rank:", mStartRankSpin);

    infoLayout->addRow("", new QLabel("<b>Description</b>"));

    mQuestDescEdit = new QTextEdit();
    mQuestDescEdit->setMaximumHeight(100);
    infoLayout->addRow("", mQuestDescEdit);

    infoLayout->addRow("", new QLabel("<b>Advanced</b>"));

    mDialogueViewEdit = new QLineEdit();
    infoLayout->addRow("Dialogue View:", mDialogueViewEdit);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    mStagesBtn = new QPushButton("Stages...");
    mAliasesBtn = new QPushButton("Aliases...");
    buttonLayout->addWidget(mStagesBtn);
    buttonLayout->addWidget(mAliasesBtn);
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &QuestEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(mStagesBtn, &QPushButton::clicked, this, &QuestEditor::openStageEditor);
    connect(mAliasesBtn, &QPushButton::clicked, this, &QuestEditor::openAliasEditor);
}

void QuestEditor::loadFromQuest()
{
    mEditorIdEdit->setText(mQuest->editorId);
    mQuestNameEdit->setText(mQuest->questName);
    mQuestTypeEdit->setText(QString::number(mQuest->questType));
    mQuestDescEdit->setPlainText(mQuest->questDesc);
    mDialogueViewEdit->setText(mQuest->dialogueView);
    mStartRankSpin->setValue(0);
}

void QuestEditor::saveRecord()
{
    if (!validateQuest())
    {
        return;
    }

    {
        auto results = ColumnValidator::validateQuest(*mQuest, mData);
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

    mQuest->editorId = mEditorIdEdit->text();
    mQuest->questName = mQuestNameEdit->text();
    mQuest->questDesc = mQuestDescEdit->toPlainText();
    mQuest->dialogueView = mDialogueViewEdit->text();
    {
        bool ok = false;
        mQuest->questType = mQuestTypeEdit->text().toUInt(&ok);
        if (!ok) mQuest->questType = 0;
    }

    accept();
}

bool QuestEditor::validateQuest()
{
    if (mQuest->editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Quest must have an EditorID.");
        return false;
    }

    if (mQuest->stageIds.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Quest must have at least one stage defined.");
        return false;
    }

    for (int s = 0; s < mQuest->stageIds.size(); ++s)
    {
        if (mQuest->stageIds[s] == 0)
        {
            QMessageBox::warning(this, "Validation Error", 
                QString("Stage at index %1 has an invalid ID (0).").arg(s));
            return false;
        }
    }

    return true;
}

void QuestEditor::openStageEditor()
{
    QuestRecord editedState = *mQuest;
    QuestStageEditor editor(&editedState, this);
    if (editor.exec() == QDialog::Accepted) {
        *mQuest = editedState;
    }
}

void QuestEditor::openAliasEditor()
{
    QuestRecord editedState = *mQuest;
    QuestAliasEditor editor(&editedState, this);
    if (editor.exec() == QDialog::Accepted) {
        *mQuest = editedState;
    }
}
