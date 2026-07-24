#include "questgrapheditor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/collection.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/tools/editrecordcommand.hpp"
#include "../../model/tools/undostack.hpp"
#include "logger.hpp"

#include "../../../libs/files/esm/questrecord.hpp"
#include "../../../libs/files/esm/esmwriter.hpp"
#include "quest_editor.hpp"

#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QHeaderView>

QuestGraphEditor::QuestGraphEditor(Data* data, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mTree(nullptr),
      mDetailEdit(nullptr),
      mSearchEdit(nullptr),
      mAddStageButton(nullptr),
      mAddObjectiveButton(nullptr),
      mEditButton(nullptr),
      mDeleteButton(nullptr),
      mSaveButton(nullptr),
      mStatusLabel(nullptr),
      mSelectedQuest(nullptr),
      mSelectedStageIndex(-1),
      mSelectedObjectiveIndex(-1),
      mSelectedAliasIndex(-1)
{
    LOG_INFO("QuestGraphEditor created");
    setupUI();
    loadQuestGraph();
}

QuestGraphEditor::~QuestGraphEditor()
{
}

void QuestGraphEditor::setupUI()
{
    setWindowTitle("Quest Graph Editor");
    setMinimumSize(1200, 800);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    auto* topBar = new QHBoxLayout();
    mSearchEdit = new QLineEdit();
    mSearchEdit->setPlaceholderText("Search quests...");
    topBar->addWidget(new QLabel("Search:"));
    topBar->addWidget(mSearchEdit, 1);
    mainLayout->addLayout(topBar);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    mTree = new QTreeWidget();
    mTree->setHeaderLabels(QStringList() << "Quest" << "Type" << "Details");
    mTree->setColumnWidth(0, 300);
    mTree->setColumnWidth(1, 80);
    mTree->setColumnWidth(2, 400);
    mTree->setAlternatingRowColors(true);
    mTree->setRootIsDecorated(true);
    splitter->addWidget(mTree);

    mDetailEdit = new QTextEdit();
    mDetailEdit->setReadOnly(true);
    mDetailEdit->setFontPointSize(10);
    splitter->addWidget(mDetailEdit);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    mainLayout->addWidget(splitter, 1);

    auto* buttonBar = new QHBoxLayout();
    mAddStageButton = new QPushButton("Add Stage");
    buttonBar->addWidget(mAddStageButton);

    mAddObjectiveButton = new QPushButton("Add Objective");
    buttonBar->addWidget(mAddObjectiveButton);

    mEditButton = new QPushButton("Edit");
    mEditButton->setEnabled(false);
    buttonBar->addWidget(mEditButton);

    mDeleteButton = new QPushButton("Delete");
    mDeleteButton->setEnabled(false);
    buttonBar->addWidget(mDeleteButton);

    buttonBar->addStretch();

    mSaveButton = new QPushButton("Save Changes");
    buttonBar->addWidget(mSaveButton);

    mainLayout->addLayout(buttonBar);

    mStatusLabel = new QLabel("Ready");
    mainLayout->addWidget(mStatusLabel);

    connect(mTree, &QTreeWidget::itemClicked, this, &QuestGraphEditor::onNodeSelected);
    connect(mAddStageButton, &QPushButton::clicked, this, &QuestGraphEditor::onAddStage);
    connect(mAddObjectiveButton, &QPushButton::clicked, this, &QuestGraphEditor::onAddObjective);
    connect(mEditButton, &QPushButton::clicked, this, &QuestGraphEditor::onEditNode);
    connect(mDeleteButton, &QPushButton::clicked, this, &QuestGraphEditor::onDeleteNode);
    connect(mSaveButton, &QPushButton::clicked, this, &QuestGraphEditor::onSave);
}

void QuestGraphEditor::loadQuestGraph()
{
    mTree->clear();
    mSelectedQuest = nullptr;
    mSelectedStageIndex = -1;
    mSelectedObjectiveIndex = -1;
    mSelectedAliasIndex = -1;

    auto& questCollection = mData->getQuestCollection();
    QVector<QString> questIds = questCollection.getIds(false);

    for (const QString& questId : questIds) {
        int idx = questCollection.getIndex(questId);
        if (idx < 0) continue;

        QuestRecord& quest = questCollection.getRecord(idx).get();
        QTreeWidgetItem* questItem = new QTreeWidgetItem(mTree);
        questItem->setText(0, quest.questName.isEmpty() ? quest.editorId : quest.questName);
        questItem->setText(1, "QUEST");
        questItem->setText(2, QString("Stages: %1 | Objectives: %2 | Aliases: %3")
            .arg(quest.stageIds.size())
            .arg(quest.objectiveIds.size())
            .arg(quest.aliasIds.size()));
        questItem->setData(0, Qt::UserRole, QVariant::fromValue<QuestRecord*>(&quest));

        // Load stages
        for (int i = 0; i < quest.stageIds.size(); ++i) {
            QTreeWidgetItem* stageItem = new QTreeWidgetItem(questItem);
            QString desc = (i < quest.stageDescriptions.size()) ? quest.stageDescriptions[i] : QString();
            stageItem->setText(0, QString("Stage %1: %2").arg(i).arg(desc));
            stageItem->setText(1, "STAGE");
            stageItem->setText(2, QString("Stage ID: 0x%1").arg(quest.stageIds[i], 8, 16, QChar('0')).toUpper());
            stageItem->setData(0, Qt::UserRole, QVariant::fromValue<int>(i));

            // Check if this stage is an objective
            bool isObjective = false;
            for (quint32 objId : quest.objectiveIds) {
                if (objId == quest.stageIds[i]) {
                    isObjective = true;
                    break;
                }
            }

            if (isObjective) {
                stageItem->setText(1, "OBJECTIVE");
                stageItem->setForeground(0, Qt::blue);
            }
        }

        // Load aliases
        for (int i = 0; i < quest.aliasIds.size(); ++i) {
            QTreeWidgetItem* aliasItem = new QTreeWidgetItem(questItem);
            aliasItem->setText(0, QString("Alias %1: 0x%2").arg(i).arg(quest.aliasIds[i], 8, 16, QChar('0')).toUpper());
            aliasItem->setText(1, "ALIAS");
            aliasItem->setText(2, QString("References quest with FormID 0x%1")
                .arg(quest.aliasIds[i], 8, 16, QChar('0')).toUpper());
            aliasItem->setData(0, Qt::UserRole, QVariant::fromValue<int>(i + 1000)); // Offset to distinguish from stages
        }
    }

    mTree->expandAll();
    mStatusLabel->setText(QString("Loaded %1 quests").arg(questIds.size()));
    LOG_INFO(QString("Loaded %1 quests").arg(questIds.size()));
}

void QuestGraphEditor::refreshTree()
{
    loadQuestGraph();
}

void QuestGraphEditor::onNodeSelected(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);

    if (!item) return;

    mEditButton->setEnabled(true);
    mDeleteButton->setEnabled(true);

    QString type = item->text(1);

    if (type == "QUEST") {
        QuestRecord* quest = static_cast<QuestRecord*>(item->data(0, Qt::UserRole).value<QuestRecord*>());
        if (quest) {
            mSelectedQuest = quest;
            showQuestDetails(quest);
        }
    } else if (type == "STAGE" || type == "OBJECTIVE") {
        int stageIndex = item->data(0, Qt::UserRole).toInt();
        mSelectedStageIndex = stageIndex;
        mSelectedObjectiveIndex = -1;
        showStageDetails(stageIndex);
    } else if (type == "ALIAS") {
        int aliasIndex = item->data(0, Qt::UserRole).toInt() - 1000;
        mSelectedAliasIndex = aliasIndex;
        mSelectedStageIndex = -1;
        showAliasDetails(aliasIndex);
    }
}

void QuestGraphEditor::showQuestDetails(const QuestRecord* quest)
{
    QString text;
    text += QString("<h2>%1</h2>").arg(quest->editorId);
    text += QString("<p><b>Quest Name:</b> %1</p>").arg(quest->questName);
    text += QString("<p><b>Description:</b> %1</p>").arg(quest->questDesc);
    text += QString("<p><b>FormID:</b> 0x%1</p>").arg(quest->formId, 8, 16, QChar('0')).toUpper();
    text += QString("<p><b>Quest Type:</b> %1</p>").arg(quest->questType);
    text += QString("<p><b>Stages:</b> %1</p>").arg(quest->stageIds.size());
    text += QString("<p><b>Objectives:</b> %1</p>").arg(quest->objectiveIds.size());
    text += QString("<p><b>Aliases:</b> %1</p>").arg(quest->aliasIds.size());
    text += QString("<p><b>Scripts:</b> %1</p>").arg(quest->scriptIds.size());

    mDetailEdit->setHtml(text);
}

void QuestGraphEditor::showStageDetails(int stageIndex)
{
    if (!mSelectedQuest || stageIndex < 0 || stageIndex >= mSelectedQuest->stageIds.size()) {
        mDetailEdit->clear();
        return;
    }

    const QuestRecord& quest = *mSelectedQuest;
    QString text;
    text += QString("<h2>Stage %1</h2>").arg(stageIndex);
    text += QString("<p><b>Stage ID:</b> 0x%1</p>")
        .arg(quest.stageIds[stageIndex], 8, 16, QChar('0')).toUpper();

    if (stageIndex < quest.stageDescriptions.size()) {
        text += QString("<p><b>Description:</b> %1</p>").arg(quest.stageDescriptions[stageIndex]);
    } else {
        text += "<p><b>Description:</b> (none)</p>";
    }

    // Check if objective
    bool isObjective = false;
    for (quint32 objId : quest.objectiveIds) {
        if (objId == quest.stageIds[stageIndex]) {
            isObjective = true;
            break;
        }
    }

    text += QString("<p><b>Is Objective:</b> %1</p>").arg(isObjective ? "Yes" : "No");

    mDetailEdit->setHtml(text);
}

void QuestGraphEditor::showObjectiveDetails(int objectiveIndex)
{
    if (!mSelectedQuest || objectiveIndex < 0 || objectiveIndex >= mSelectedQuest->objectiveIds.size()) {
        mDetailEdit->clear();
        return;
    }

    const QuestRecord& quest = *mSelectedQuest;
    quint32 stageId = quest.objectiveIds[objectiveIndex];

    QString text;
    text += QString("<h2>Objective %1</h2>").arg(objectiveIndex);
    text += QString("<p><b>Stage ID:</b> 0x%1</p>").arg(stageId, 8, 16, QChar('0')).toUpper();

    // Find corresponding stage
    for (int i = 0; i < quest.stageIds.size(); ++i) {
        if (quest.stageIds[i] == stageId) {
            text += QString("<p><b>Stage %1 Description:</b> %2</p>")
                .arg(i)
                .arg((i < quest.stageDescriptions.size()) ? quest.stageDescriptions[i] : "(none)");
            break;
        }
    }

    mDetailEdit->setHtml(text);
}

void QuestGraphEditor::showAliasDetails(int aliasIndex)
{
    if (!mSelectedQuest || aliasIndex < 0 || aliasIndex >= mSelectedQuest->aliasIds.size()) {
        mDetailEdit->clear();
        return;
    }

    const QuestRecord& quest = *mSelectedQuest;
    quint32 aliasFormId = quest.aliasIds[aliasIndex];

    QString text;
    text += QString("<h2>Alias %1</h2>").arg(aliasIndex);
    text += QString("<p><b>FormID:</b> 0x%1</p>").arg(aliasFormId, 8, 16, QChar('0')).toUpper();

    // Try to find the referenced quest
    auto& aliasQuestCollection = mData->getQuestCollection();
    QVector<QString> aliasQuestIds = aliasQuestCollection.getIds(false);
    bool found = false;
    for (const QString& aliasId : aliasQuestIds) {
        int aliasIdx = aliasQuestCollection.getIndex(aliasId);
        if (aliasIdx >= 0) {
            const QuestRecord& aliasQuest = aliasQuestCollection.getRecord(aliasIdx).get();
            if (aliasQuest.formId == aliasFormId) {
                text += QString("<p><b>Referenced Quest:</b> %1 (%2)</p>")
                    .arg(aliasQuest.editorId)
                    .arg(aliasQuest.questName);
                found = true;
                break;
            }
        }
    }

    if (!found) {
        text += "<p><b>Referenced Quest:</b> (not found in current data)</p>";
    }

    mDetailEdit->setHtml(text);
}

void QuestGraphEditor::onAddStage()
{
    if (!mSelectedQuest) {
        QMessageBox::information(this, "No Selection", "Please select a quest first.");
        return;
    }

    bool ok = false;
    QString description = QInputDialog::getText(this, "Add Stage",
        "Enter stage description:", QLineEdit::Normal, "", &ok);

    if (!ok || description.isEmpty()) return;

    // Add new stage ID (use a simple incrementing scheme)
    quint32 newStageId = mSelectedQuest->stageIds.isEmpty() ? 1 : mSelectedQuest->stageIds.last() + 1;
    mSelectedQuest->stageIds.append(newStageId);
    mSelectedQuest->stageDescriptions.append(description);

    LOG_INFO(QString("Added stage %1 to quest '%2'").arg(mSelectedQuest->stageIds.size() - 1).arg(mSelectedQuest->editorId));
    mStatusLabel->setText(QString("Added stage %1").arg(mSelectedQuest->stageIds.size() - 1));
    refreshTree();
}

void QuestGraphEditor::onAddObjective()
{
    if (!mSelectedQuest || mSelectedStageIndex < 0) {
        QMessageBox::information(this, "No Selection", "Please select a stage first.");
        return;
    }

    quint32 stageId = mSelectedQuest->stageIds[mSelectedStageIndex];

    // Check if already an objective
    for (quint32 objId : mSelectedQuest->objectiveIds) {
        if (objId == stageId) {
            QMessageBox::information(this, "Already Objective", "This stage is already an objective.");
            return;
        }
    }

    mSelectedQuest->objectiveIds.append(stageId);

    LOG_INFO(QString("Added stage %1 as objective to quest '%2'")
        .arg(mSelectedStageIndex).arg(mSelectedQuest->editorId));
    mStatusLabel->setText(QString("Added objective"));
    refreshTree();
}

void QuestGraphEditor::onEditNode()
{
    QTreeWidgetItem* item = mTree->currentItem();
    if (!item) return;

    QString type = item->text(1);

    if (type == "QUEST") {
        const QuestRecord* quest = static_cast<const QuestRecord*>(item->data(0, Qt::UserRole).value<const QuestRecord*>());
        if (!quest) return;

        QuestRecord originalState = *quest;
        QuestRecord editedState = originalState;
        QuestEditor editor(mData, &editedState, this);
        if (editor.exec() == QDialog::Accepted) {
            auto& coll = mData->getQuestCollection();
            int idx = coll.searchId(editedState.editorId);
            if (idx >= 0 && mData->getUndoStack()) {
                EditRecordCommand<QuestRecord>* cmd = new EditRecordCommand<QuestRecord>(&coll, idx, originalState, editedState,
                    "Edit quest: " + editedState.editorId);
                if (cmd->hasChanged()) {
                    mData->getUndoStack()->push(cmd);
                } else {
                    delete cmd;
                }
            }
            LOG_INFO(QString("Updated quest '%1'").arg(editedState.editorId));
            refreshTree();
        }
    } else if (type == "STAGE" || type == "OBJECTIVE") {
        int stageIndex = item->data(0, Qt::UserRole).toInt();
        if (!mSelectedQuest || stageIndex < 0 || stageIndex >= mSelectedQuest->stageIds.size()) return;

        QuestRecord originalState = *mSelectedQuest;
        QString oldDesc = (stageIndex < mSelectedQuest->stageDescriptions.size()) ? mSelectedQuest->stageDescriptions[stageIndex] : "";

        bool ok = false;
        QString newDesc = QInputDialog::getText(this, "Edit Stage",
            QString("Enter new description for stage %1:").arg(stageIndex),
            QLineEdit::Normal,
            oldDesc,
            &ok);

        if (!ok) return;

        if (stageIndex >= mSelectedQuest->stageDescriptions.size()) {
            mSelectedQuest->stageDescriptions.resize(stageIndex + 1);
        }
        mSelectedQuest->stageDescriptions[stageIndex] = newDesc;

        auto& coll = mData->getQuestCollection();
        int idx = coll.searchId(mSelectedQuest->editorId);
        if (idx >= 0 && mData->getUndoStack() && oldDesc != newDesc) {
            EditRecordCommand<QuestRecord>* cmd = new EditRecordCommand<QuestRecord>(&coll, idx, originalState, *mSelectedQuest,
                "Edit quest stage: " + mSelectedQuest->editorId);
            if (cmd->hasChanged()) {
                mData->getUndoStack()->push(cmd);
            } else {
                delete cmd;
            }
        }

        LOG_INFO(QString("Updated stage %1 description in quest '%2'").arg(stageIndex).arg(mSelectedQuest->editorId));
        refreshTree();
    } else if (type == "ALIAS") {
        int aliasIndex = item->data(0, Qt::UserRole).toInt() - 1000;
        if (!mSelectedQuest || aliasIndex < 0 || aliasIndex >= mSelectedQuest->aliasIds.size()) return;

        QuestRecord originalState = *mSelectedQuest;
        quint32 oldFormId = mSelectedQuest->aliasIds[aliasIndex];

        bool ok = false;
        QString newFormIdStr = QInputDialog::getText(this, "Edit Alias",
            QString("Enter new FormID (hex) for alias %1:").arg(aliasIndex),
            QLineEdit::Normal,
            QString("0x%1").arg(oldFormId, 8, 16, QChar('0')).toUpper(),
            &ok);

        if (!ok) return;

        bool convOk;
        quint32 newFormId = newFormIdStr.remove("0x").remove("0X").toUInt(&convOk, 16);
        if (!convOk) {
            QMessageBox::critical(this, "Error", "Invalid hex FormID.");
            return;
        }

        mSelectedQuest->aliasIds[aliasIndex] = newFormId;

        auto& coll = mData->getQuestCollection();
        int idx = coll.searchId(mSelectedQuest->editorId);
        if (idx >= 0 && mData->getUndoStack() && oldFormId != newFormId) {
            EditRecordCommand<QuestRecord>* cmd = new EditRecordCommand<QuestRecord>(&coll, idx, originalState, *mSelectedQuest,
                "Edit quest alias: " + mSelectedQuest->editorId);
            if (cmd->hasChanged()) {
                mData->getUndoStack()->push(cmd);
            } else {
                delete cmd;
            }
        }

        LOG_INFO(QString("Updated alias %1 FormID in quest '%2'").arg(aliasIndex).arg(mSelectedQuest->editorId));
        refreshTree();
    }
}

void QuestGraphEditor::onDeleteNode()
{
    QTreeWidgetItem* item = mTree->currentItem();
    if (!item) return;

    QString type = item->text(1);

    if (type == "STAGE" || type == "OBJECTIVE") {
        int stageIndex = item->data(0, Qt::UserRole).toInt();
        if (!mSelectedQuest || stageIndex < 0 || stageIndex >= mSelectedQuest->stageIds.size()) return;

        auto reply = QMessageBox::question(this, "Delete Stage",
            QString("Are you sure you want to delete stage %1?\n\nThis action cannot be undone.")
                .arg(stageIndex),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            mSelectedQuest->stageIds.removeAt(stageIndex);
            if (stageIndex < mSelectedQuest->stageDescriptions.size()) {
                mSelectedQuest->stageDescriptions.removeAt(stageIndex);
            }
            // Remove from objectives if present
            for (int i = 0; i < mSelectedQuest->objectiveIds.size(); ++i) {
                if (mSelectedQuest->objectiveIds[i] == stageIndex) {
                    mSelectedQuest->objectiveIds.removeAt(i);
                    break;
                }
            }
            LOG_INFO(QString("Deleted stage %1 from quest '%2'").arg(stageIndex).arg(mSelectedQuest->editorId));
            refreshTree();
        }
    } else if (type == "ALIAS") {
        int aliasIndex = item->data(0, Qt::UserRole).toInt() - 1000;
        if (!mSelectedQuest || aliasIndex < 0 || aliasIndex >= mSelectedQuest->aliasIds.size()) return;

        auto reply = QMessageBox::question(this, "Delete Alias",
            QString("Are you sure you want to delete alias %1?\n\nThis action cannot be undone.")
                .arg(aliasIndex),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            mSelectedQuest->aliasIds.removeAt(aliasIndex);
            LOG_INFO(QString("Deleted alias %1 from quest '%2'").arg(aliasIndex).arg(mSelectedQuest->editorId));
            refreshTree();
        }
    }
}

void QuestGraphEditor::onSave()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save Quest Graph", "",
        "ESM Files (*.esm);;All Files (*)");

    if (filePath.isEmpty()) return;

    ESMWriter writer;
    QFile saveFile(filePath);
    if (!saveFile.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "Error", QString("Cannot open file: %1").arg(filePath));
        return;
    }

    const auto& metaData = mData->getMetaData().getRecords();
    for (const auto& record : metaData)
    {
        writer.addMaster(record.get().editorId);
    }

    writer.setVersion(1.0f);

    int totalQuests = 0;
    int totalStages = 0;
    int totalObjectives = 0;
    int totalAliases = 0;

    const auto& questRecords = mData->getQuestCollection().getRecords();
    for (const auto& record : questRecords)
    {
        if (record.state == State_Modified || record.state == State_ModifiedOnly)
        {
            RecHeader recHeader;
            recHeader.id = record.get().formId;
            writer.startRecord('QUST', recHeader);
            record.get().save(writer);
            writer.endRecord();
            totalQuests++;
            totalStages += record.get().stageIds.size();
            totalObjectives += record.get().objectiveIds.size();
            totalAliases += record.get().aliasIds.size();
        }
    }

    writer.close();
    saveFile.close();

    LOG_INFO(QString("Saved quest graph to %1").arg(filePath));
    LOG_INFO(QString("Quests: %1, Stages: %2, Objectives: %3, Aliases: %4")
        .arg(totalQuests).arg(totalStages).arg(totalObjectives).arg(totalAliases));

    QMessageBox::information(this, "Saved",
        QString("Quest graph data exported.\n\n"
                "Quests: %1\n"
                "Stages: %2\n"
                "Objectives: %3\n"
                "Aliases: %4")
            .arg(totalQuests)
            .arg(totalStages)
            .arg(totalObjectives)
            .arg(totalAliases));
}
