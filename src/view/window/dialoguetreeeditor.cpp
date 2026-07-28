#include "dialoguetreeeditor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/collection.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/tools/editrecordcommand.hpp"
#include "../../model/tools/undostack.hpp"
#include "logger.hpp"

#include "../../../libs/files/esm/dialrecord.hpp"
#include "../../../libs/files/esm/inforecord.hpp"
#include "../../../libs/files/esm/esmwriter.hpp"
#include "dialeditor.hpp"
#include "infoeditor.hpp"

#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QHeaderView>
#include <QDateTime>

DialogueTreeEditor::DialogueTreeEditor(Data* data, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mTree(nullptr),
      mDetailEdit(nullptr),
      mSearchEdit(nullptr),
      mAddDialButton(nullptr),
      mAddInfoButton(nullptr),
      mEditButton(nullptr),
      mDeleteButton(nullptr),
      mSaveButton(nullptr),
      mStatusLabel(nullptr)
{
    LOG_INFO("DialogueTreeEditor created");
    setupUI();
    loadDialogueTree();
}

DialogueTreeEditor::~DialogueTreeEditor()
{
}

void DialogueTreeEditor::setupUI()
{
    setWindowTitle("Dialogue Tree Editor");
    setMinimumSize(1200, 800);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    auto* topBar = new QHBoxLayout();
    mSearchEdit = new QLineEdit();
    mSearchEdit->setPlaceholderText("Search dialogues...");
    topBar->addWidget(new QLabel("Search:"));
    topBar->addWidget(mSearchEdit, 1);
    mainLayout->addLayout(topBar);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    mTree = new QTreeWidget();
    mTree->setHeaderLabels(QStringList() << "Dialogue" << "Type" << "Details");
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
    mAddDialButton = new QPushButton("Add Dialogue");
    buttonBar->addWidget(mAddDialButton);

    mAddInfoButton = new QPushButton("Add Response");
    buttonBar->addWidget(mAddInfoButton);

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

    connect(mTree, &QTreeWidget::itemClicked, this, &DialogueTreeEditor::onNodeSelected);
    connect(mAddDialButton, &QPushButton::clicked, this, &DialogueTreeEditor::onAddDial);
    connect(mAddInfoButton, &QPushButton::clicked, this, &DialogueTreeEditor::onAddInfo);
    connect(mEditButton, &QPushButton::clicked, this, &DialogueTreeEditor::onEditNode);
    connect(mDeleteButton, &QPushButton::clicked, this, &DialogueTreeEditor::onDeleteNode);
    connect(mSaveButton, &QPushButton::clicked, this, &DialogueTreeEditor::onSave);
}

void DialogueTreeEditor::loadDialogueTree()
{
    mTree->clear();
    mSelectedDials.clear();
    mSelectedInfos.clear();

    auto& dialCollection = mData->getDialCollection();
    QVector<QString> dialIds = dialCollection.getIds(false);

    for (const QString& dialId : dialIds) {
        int idx = dialCollection.getIndex(dialId);
        if (idx < 0) continue;

        DialRecord& dial = dialCollection.getRecord(idx).get();
        QTreeWidgetItem* dialItem = new QTreeWidgetItem(mTree);
        dialItem->setText(0, dial.topicName.isEmpty() ? dial.editorId : dial.topicName);
        dialItem->setText(1, "DIAL");
        dialItem->setText(2, QString("Topic: %1").arg(dial.topicName));
        dialItem->setData(0, Qt::UserRole, QVariant::fromValue<DialRecord*>(&dial));

        // Load INFO children
        auto& infoCollection = mData->getInfoCollection();
        for (quint32 responseId : dial.responseIds) {
            QString infoEditorId;
            // Find INFO by formId
            QVector<QString> infoIds = infoCollection.getIds(false);
            for (const QString& infoId : infoIds) {
                int infoIdx = infoCollection.getIndex(infoId);
                if (infoIdx >= 0) {
                    const InfoRecord& info = infoCollection.getRecord(infoIdx).get();
                    if (info.formId == responseId) {
                        infoEditorId = infoId;
                        break;
                    }
                }
            }

                    if (!infoEditorId.isEmpty()) {
                        int infoIdx = infoCollection.getIndex(infoEditorId);
                        if (infoIdx >= 0) {
                            InfoRecord& info = infoCollection.getRecord(infoIdx).get();
                            QTreeWidgetItem* infoItem = new QTreeWidgetItem(dialItem);
                            infoItem->setText(0, info.responseText.left(50));
                            infoItem->setText(1, "INFO");
                            infoItem->setText(2, QString("Response: %1...").arg(info.responseText.left(50)));
                            infoItem->setData(0, Qt::UserRole, QVariant::fromValue<InfoRecord*>(&info));
                        }
                    }
        }
    }

    mTree->expandAll();
    mStatusLabel->setText(QString("Loaded %1 dialogues").arg(dialIds.size()));
    LOG_INFO(QString("Loaded %1 dialogues").arg(dialIds.size()));
}

void DialogueTreeEditor::refreshTree()
{
    loadDialogueTree();
}

void DialogueTreeEditor::onNodeSelected(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);

    if (!item) return;

    mEditButton->setEnabled(true);
    mDeleteButton->setEnabled(true);

    int type = getTreeWidgetItemType(item);

    if (type == 0) { // DIAL
        DialRecord* dial = static_cast<DialRecord*>(item->data(0, Qt::UserRole).value<DialRecord*>());
        if (dial) {
            showDialDetails(dial);
            mSelectedDials.clear();
            mSelectedDials.append(dial);
        }
    } else if (type == 1) { // INFO
        InfoRecord* info = static_cast<InfoRecord*>(item->data(0, Qt::UserRole).value<InfoRecord*>());
        if (info) {
            showInfoDetails(info);
            mSelectedInfos.clear();
            mSelectedInfos.append(info);
        }
    }
}

void DialogueTreeEditor::showDialDetails(const DialRecord* dial)
{
    QString text;
    text += QString("<h2>%1</h2>").arg(dial->editorId);
    text += QString("<p><b>Topic:</b> %1</p>").arg(dial->topicName);
    text += QString("<p><b>FormID:</b> 0x%1</p>").arg(dial->formId, 8, 16, QChar('0')).toUpper();
    text += QString("<p><b>Responses:</b> %1</p>").arg(dial->responseIds.size());
    text += QString("<p><b>Conditions:</b> %1</p>").arg(dial->conditionIds.size());

    mDetailEdit->setHtml(text);
}

void DialogueTreeEditor::showInfoDetails(const InfoRecord* info)
{
    QString text;
    text += QString("<h2>%1</h2>").arg(info->editorId);
    text += QString("<p><b>Response Text:</b></p>");
    text += QString("<p>%1</p>").arg(info->responseText);
    text += QString("<p><b>FormID:</b> 0x%1</p>").arg(info->formId, 8, 16, QChar('0')).toUpper();
    text += QString("<p><b>Target ID:</b> 0x%1</p>").arg(info->targetId, 8, 16, QChar('0')).toUpper();
    text += QString("<p><b>Conditions:</b> %1</p>").arg(info->conditions.size());
    if (!info->conditions.isEmpty())
    {
        text += QStringLiteral("<ul>");
        for (const auto& c : info->conditions)
            text += QString("<li>%1 %2 %3 <i>(%4)</i></li>")
                .arg(c.function, c.comparison, c.value.toString(),
                     c.useAND ? QStringLiteral("AND") : QStringLiteral("OR"));
        text += QStringLiteral("</ul>");
    }
    text += QString("<p><b>Scripts:</b> %1</p>").arg(info->scriptIds.size());
    if (!info->voiceFile.isEmpty())
        text += QString("<p><b>Voice File:</b> %1</p>").arg(info->voiceFile);
    if (!info->scriptFragment.isEmpty())
        text += QString("<p><b>Script Fragment:</b> <pre>%1</pre></p>")
            .arg(info->scriptFragment.toHtmlEscaped());

    mDetailEdit->setHtml(text);
}

QString DialogueTreeEditor::getTreeWidgetItemText(QTreeWidgetItem* item) const
{
    if (!item) return QString();
    return item->text(0);
}

int DialogueTreeEditor::getTreeWidgetItemType(QTreeWidgetItem* item) const
{
    if (!item) return -1;
    QString type = item->text(1);
    if (type == "DIAL") return 0;
    if (type == "INFO") return 1;
    return -1;
}

void DialogueTreeEditor::onAddDial()
{
    bool ok = false;
    QString editorId = QInputDialog::getText(this, "Add Dialogue",
        "Enter Editor ID for new dialogue:", QLineEdit::Normal, "", &ok);

    if (!ok || editorId.isEmpty()) return;

    DialRecord newDial;
    newDial.editorId = editorId.toLower();
    newDial.formId = 0;
    newDial.topicName = editorId;

    if (mData->addDial(newDial)) {
        LOG_INFO(QString("Added dialogue '%1'").arg(editorId));
        mStatusLabel->setText(QString("Added dialogue '%1'").arg(editorId));
        refreshTree();
    } else {
        QMessageBox::warning(this, "Error",
            QString("Failed to add dialogue '%1'. ID may already exist.").arg(editorId));
    }
}

void DialogueTreeEditor::onAddInfo()
{
    if (mSelectedDials.isEmpty()) {
        QMessageBox::information(this, "No Selection",
            "Please select a DIAL node first.");
        return;
    }

    bool ok = false;
    QString responseText = QInputDialog::getMultiLineText(this, "Add Response",
        "Enter response text:", "", &ok);

    if (!ok || responseText.isEmpty()) return;

    InfoRecord newInfo;
    newInfo.editorId = QString("Response_%1").arg(QDateTime::currentMSecsSinceEpoch() % 100000);
    newInfo.formId = 0;
    newInfo.responseText = responseText;

    if (mData->addInfo(newInfo)) {
        LOG_INFO(QString("Added info '%1'").arg(newInfo.editorId));
        mStatusLabel->setText(QString("Added response '%1'").arg(newInfo.editorId));
        refreshTree();
    } else {
        QMessageBox::warning(this, "Error",
            QString("Failed to add response."));
    }
}

void DialogueTreeEditor::onEditNode()
{
    QTreeWidgetItem* item = mTree->currentItem();
    if (!item) return;

    int type = getTreeWidgetItemType(item);

    if (type == 0) { // DIAL
        const DialRecord* dial = static_cast<const DialRecord*>(item->data(0, Qt::UserRole).value<const DialRecord*>());
        if (!dial) return;

        DialRecord originalState = *dial;
        DialRecord editedState = originalState;
        DialEditor editor(mData, this);
        editor.loadRecord(&editedState);
        if (editor.exec() == QDialog::Accepted) {
            auto& coll = mData->getDialCollection();
            int idx = coll.searchId(dial->editorId);
            if (idx >= 0 && mData->getUndoStack()) {
                EditRecordCommand<DialRecord>* cmd = new EditRecordCommand<DialRecord>(&coll, idx, originalState, editedState,
                    "Edit dialogue: " + dial->editorId);
                if (cmd->hasChanged()) {
                    mData->getUndoStack()->push(cmd);
                } else {
                    delete cmd;
                }
            }
            LOG_INFO(QString("Updated dialogue '%1'").arg(dial->editorId));
            refreshTree();
        }
    } else if (type == 1) { // INFO
        const InfoRecord* info = static_cast<const InfoRecord*>(item->data(0, Qt::UserRole).value<const InfoRecord*>());
        if (!info) return;

        InfoRecord originalState = *info;
        InfoRecord editedState = originalState;
        InfoEditor editor(mData, this);
        editor.loadRecord(&editedState);
        if (editor.exec() == QDialog::Accepted) {
            auto& coll = mData->getInfoCollection();
            int idx = coll.searchId(info->editorId);
            if (idx >= 0 && mData->getUndoStack()) {
                EditRecordCommand<InfoRecord>* cmd = new EditRecordCommand<InfoRecord>(&coll, idx, originalState, editedState,
                    "Edit response: " + info->editorId);
                if (cmd->hasChanged()) {
                    mData->getUndoStack()->push(cmd);
                } else {
                    delete cmd;
                }
            }
            LOG_INFO(QString("Updated response '%1'").arg(info->editorId));
            refreshTree();
        }
    }
}

void DialogueTreeEditor::onDeleteNode()
{
    QTreeWidgetItem* item = mTree->currentItem();
    if (!item) return;

    int type = getTreeWidgetItemType(item);

    if (type == 0) { // Delete DIAL
        QString dialId = getTreeWidgetItemText(item);
        auto reply = QMessageBox::question(this, "Delete Dialogue",
            QString("Are you sure you want to delete dialogue '%1'?\n\nThis action cannot be undone.")
                .arg(dialId),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            mData->removeRecord(CkId::Type_Dial_, dialId);
            LOG_INFO(QString("Deleted dialogue '%1'").arg(dialId));
            refreshTree();
        }
    } else if (type == 1) { // Delete INFO
        QString infoId = getTreeWidgetItemText(item);
        auto reply = QMessageBox::question(this, "Delete Response",
            QString("Are you sure you want to delete response '%1'?\n\nThis action cannot be undone.")
                .arg(infoId),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            mData->removeRecord(CkId::Type_Info_, infoId);
            LOG_INFO(QString("Deleted response '%1'").arg(infoId));
            refreshTree();
        }
    }
}

void DialogueTreeEditor::onSave()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save Dialogue Tree", "",
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

    int dialCount = 0;
    int infoCount = 0;

    const auto& dials = mData->getDialCollection().getRecords();
    for (const auto& record : dials)
    {
        if (record.state == State_Modified || record.state == State_ModifiedOnly)
        {
            RecHeader recHeader;
            recHeader.id = record.get().formId;
            writer.startRecord('DIAL', recHeader);
            record.get().save(writer);
            writer.endRecord();
            dialCount++;
        }
    }

    const auto& infos = mData->getInfoCollection().getRecords();
    for (const auto& record : infos)
    {
        if (record.state == State_Modified || record.state == State_ModifiedOnly)
        {
            RecHeader recHeader;
            recHeader.id = record.get().formId;
            writer.startRecord('INFO', recHeader);
            record.get().save(writer);
            writer.endRecord();
            infoCount++;
        }
    }

    writer.close();
    saveFile.close();

    LOG_INFO(QString("Saved dialogue tree to %1").arg(filePath));
    LOG_INFO(QString("DIAL records: %1, INFO records: %2").arg(dialCount).arg(infoCount));

    QMessageBox::information(this, "Saved",
        QString("Dialogue tree data exported.\n\n"
                "DIAL records: %1\n"
                "INFO records: %2")
            .arg(dialCount)
            .arg(infoCount));
}
