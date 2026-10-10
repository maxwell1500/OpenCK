#include "dialoguetreeeditor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/collection.hpp"
#include "../../model/world/collection_impl.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/tools/editrecordcommand.hpp"
#include "../../model/tools/addrecordcommand.hpp"
#include "../../model/tools/undostack.hpp"
#include "../../model/tools/macrocommand.hpp"
#include "../../model/tools/setinfoparentdialcommand.hpp"
#include "logger.hpp"

#include "../../../libs/files/esm/dialrecord.hpp"
#include "../../../libs/files/esm/inforecord.hpp"
#include "dialeditor.hpp"
#include "infoeditor.hpp"

#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QHeaderView>
#include <QDateTime>
#include <QHash>
#include <utility>

DialogueTreeEditor::DialogueTreeEditor(Data* data, std::function<bool()> saveCallback,
                                       QWidget* parent)
    : QDialog(parent),
      mData(data),
      mSaveCallback(std::move(saveCallback)),
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

    // Index INFO records by form id once per tree build: resolving each
    // response with a fresh getIds()/getIndex() scan is O(dials x
    // responses x infos) and hangs on full-master dialogue. First
    // occurrence wins, matching the old scan order.
    auto& infoCollection = mData->getInfoCollection();
    QHash<quint32, int> infoByForm;
    infoByForm.reserve(infoCollection.size());
    for (int i = 0; i < infoCollection.size(); i++) {
        const quint32 fid = infoCollection.getRecord(i).get().formId;
        if (!infoByForm.contains(fid))
            infoByForm.insert(fid, i);
    }

    for (const QString& dialId : dialIds) {
        int idx = dialCollection.getIndex(dialId);
        if (idx < 0) continue;

        DialRecord& dial = dialCollection.getRecord(idx).get();
        QTreeWidgetItem* dialItem = new QTreeWidgetItem(mTree);
        dialItem->setText(0, dial.topicName.isEmpty() ? dial.editorId : dial.topicName);
        dialItem->setText(1, "DIAL");
        dialItem->setText(2, QString("Topic: %1").arg(dial.topicName));
        dialItem->setData(0, Qt::UserRole, QVariant::fromValue<DialRecord*>(&dial));
        dialItem->setData(0, Qt::UserRole + 1, dial.editorId);
        dialItem->setData(0, Qt::UserRole + 2, dial.formId);
        // Load INFO children
        for (quint32 responseId : dial.responseIds) {
            const int infoIdx = infoByForm.value(responseId, -1);
            if (infoIdx < 0)
                continue;
            InfoRecord& info = infoCollection.getRecord(infoIdx).get();
            QTreeWidgetItem* infoItem = new QTreeWidgetItem(dialItem);
            infoItem->setText(0, info.responseText.left(50));
            infoItem->setText(1, "INFO");
            infoItem->setText(2, QString("Response: %1...").arg(info.responseText.left(50)));
            infoItem->setData(0, Qt::UserRole, QVariant::fromValue<InfoRecord*>(&info));
            infoItem->setData(0, Qt::UserRole + 1, info.editorId);
            infoItem->setData(0, Qt::UserRole + 2, info.formId);
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
                .arg(CtdaCondition::comparisonName(c.comparison),
                     CtdaCondition::functionName(c.functionId),
                     QString::number(c.param1),
                     c.useOr() ? QStringLiteral("OR") : QStringLiteral("AND"));
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
    QString editorId = QInputDialog::getText(this, tr("Add Dialogue"),
        tr("Enter Editor ID for new dialogue:"), QLineEdit::Normal, "", &ok);

    if (!ok || editorId.trimmed().isEmpty()) return;
    const QString finalId = editorId.trimmed();

    auto& dialCollection = mData->getDialCollection();
    if (dialCollection.searchId(finalId) >= 0)
    {
        QMessageBox::warning(this, tr("Add Dialogue"),
            tr("A dialogue named '%1' already exists.").arg(finalId));
        return;
    }

    DialRecord newDial;
    newDial.blank();
    newDial.initComponents();
    newDial.editorId = finalId;
    newDial.topicName = finalId;
    try
    {
        newDial.formId = mData->createNewRecord(CkId::Type_Dial_, finalId);
    }
    catch (const std::exception& e)
    {
        QMessageBox::warning(this, tr("Add Dialogue"),
            tr("Could not allocate a FormID: %1").arg(QString::fromUtf8(e.what())));
        return;
    }

    auto* table = qobject_cast<IdTable*>(mData->getTableModel(CkId::Type_Dial_));
    const int appendIdx = dialCollection.getAppendIndex(finalId, CkId::Type_Dial_);
    Record<DialRecord> record(State_ModifiedOnly, nullptr, &newDial);

    if (mData->getUndoStack() && table)
    {
        mData->getUndoStack()->push(new AddRecordCommand(
            table, &dialCollection, appendIdx, record,
            QStringLiteral("Add Dialogue: %1").arg(finalId)));
    }
    else
    {
        dialCollection.appendRecord(record, CkId::Type_Dial_);
    }

    LOG_INFO(QString("Added dialogue '%1'").arg(finalId));
    mStatusLabel->setText(QString("Added dialogue '%1'").arg(finalId));
    refreshTree();
}

void DialogueTreeEditor::onAddInfo()
{
    QTreeWidgetItem* current = mTree->currentItem();
    QTreeWidgetItem* dialItem = nullptr;
    if (current)
    {
        if (getTreeWidgetItemType(current) == 0)
            dialItem = current;
        else if (getTreeWidgetItemType(current) == 1 && current->parent())
            dialItem = current->parent();
    }
    if (!dialItem)
    {
        QMessageBox::information(this, tr("No Selection"),
            tr("Please select a DIAL node or response first."));
        return;
    }

    const QString parentDialId = dialItem->data(0, Qt::UserRole + 1).toString();
    auto& dialCollection = mData->getDialCollection();
    int dialIdx = dialCollection.searchId(parentDialId);
    if (dialIdx < 0)
    {
        QMessageBox::warning(this, tr("Add Response"), tr("Selected dialogue could not be found."));
        return;
    }

    bool ok = false;
    QString responseText = QInputDialog::getMultiLineText(this, tr("Add Response"),
        tr("Enter response text:"), "", &ok);

    if (!ok || responseText.trimmed().isEmpty()) return;

    QString infoId;
    auto& infoCollection = mData->getInfoCollection();
    for (int attempt = 0; attempt < 100; ++attempt)
    {
        infoId = QString("Response_%1_%2")
            .arg(QDateTime::currentMSecsSinceEpoch() % 100000)
            .arg(attempt);
        if (infoCollection.searchId(infoId) < 0)
            break;
    }

    InfoRecord newInfo;
    newInfo.blank();
    newInfo.initComponents();
    newInfo.editorId = infoId;
    newInfo.responseText = responseText.trimmed();
    try
    {
        newInfo.formId = mData->createNewRecord(CkId::Type_Info_, infoId);
    }
    catch (const std::exception& e)
    {
        QMessageBox::warning(this, tr("Add Response"),
            tr("Could not allocate a FormID: %1").arg(QString::fromUtf8(e.what())));
        return;
    }

    DialRecord origDial = dialCollection.getRecord(dialIdx).get();
    DialRecord updatedDial = origDial;
    updatedDial.responseIds.append(newInfo.formId);

    auto* infoTable = qobject_cast<IdTable*>(mData->getTableModel(CkId::Type_Info_));
    const int appendIdx = infoCollection.getAppendIndex(infoId, CkId::Type_Info_);
    Record<InfoRecord> infoRec(State_ModifiedOnly, nullptr, &newInfo);

    if (mData->getUndoStack() && infoTable)
    {
        auto* macro = new MacroCommand(QStringLiteral("Add Response to %1").arg(parentDialId));
        macro->addCommand(new AddRecordCommand(
            infoTable, &infoCollection, appendIdx, infoRec,
            QStringLiteral("Add response %1").arg(infoId)));
        macro->addCommand(new EditRecordCommand<DialRecord>(
            &dialCollection, dialIdx, origDial, updatedDial,
            QStringLiteral("Link response to %1").arg(parentDialId)));
        macro->addCommand(new SetInfoParentDialCommand(
            mData, newInfo.formId, 0, origDial.formId));
        mData->getUndoStack()->push(macro);
    }
    else
    {
        infoCollection.appendRecord(infoRec, CkId::Type_Info_);
        dialCollection.getRecord(dialIdx).get().responseIds.append(newInfo.formId);
        dialCollection.getRecord(dialIdx).state = State_Modified;
    }

    LOG_INFO(QString("Added response '%1' to dialogue '%2'").arg(infoId, parentDialId));
    mStatusLabel->setText(QString("Added response '%1'").arg(infoId));
    refreshTree();
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
        QString dialId = item->data(0, Qt::UserRole + 1).toString();
        if (dialId.isEmpty()) dialId = getTreeWidgetItemText(item);

        auto reply = QMessageBox::question(this, tr("Delete Dialogue"),
            tr("Are you sure you want to delete dialogue '%1'?").arg(dialId),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            auto& coll = mData->getDialCollection();
            bool removed = coll.removeRecordWithUndo(dialId, mData->getUndoStack());
            if (!removed)
                mData->removeRecord(CkId::Type_Dial_, dialId);
            LOG_INFO(QString("Deleted dialogue '%1'").arg(dialId));
            mSelectedDials.clear();
            refreshTree();
        }
    } else if (type == 1) { // Delete INFO
        QString infoId = item->data(0, Qt::UserRole + 1).toString();
        quint32 infoFormId = item->data(0, Qt::UserRole + 2).toUInt();
        if (infoId.isEmpty()) infoId = getTreeWidgetItemText(item);

        auto reply = QMessageBox::question(this, tr("Delete Response"),
            tr("Are you sure you want to delete response '%1'?").arg(infoId),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            QTreeWidgetItem* parentItem = item->parent();
            QString parentDialId = parentItem ? parentItem->data(0, Qt::UserRole + 1).toString() : QString();

            auto& dialCollection = mData->getDialCollection();
            auto& infoCollection = mData->getInfoCollection();

            int dialIdx = parentDialId.isEmpty() ? -1 : dialCollection.searchId(parentDialId);

            if (mData->getUndoStack())
            {
                if (dialIdx >= 0)
                {
                    DialRecord origDial = dialCollection.getRecord(dialIdx).get();
                    DialRecord updatedDial = origDial;
                    updatedDial.responseIds.removeAll(infoFormId);
                    if (origDial != updatedDial)
                    {
                        mData->getUndoStack()->push(new EditRecordCommand<DialRecord>(
                            &dialCollection, dialIdx, origDial, updatedDial,
                            QStringLiteral("Unlink response from %1").arg(parentDialId)));
                    }
                    mData->getUndoStack()->push(new SetInfoParentDialCommand(
                        mData, infoFormId, origDial.formId, 0));
                }
                infoCollection.removeRecordWithUndo(infoId, mData->getUndoStack());
            }
            else
            {
                if (dialIdx >= 0)
                {
                    dialCollection.getRecord(dialIdx).get().responseIds.removeAll(infoFormId);
                    dialCollection.getRecord(dialIdx).state = State_Modified;
                }
                infoCollection.removeRecordWithUndo(infoId, nullptr);
            }

            LOG_INFO(QString("Deleted response '%1'").arg(infoId));
            mSelectedInfos.clear();
            refreshTree();
        }
    }
}

void DialogueTreeEditor::onSave()
{
    if (!mSaveCallback || !mSaveCallback())
    {
        QMessageBox::warning(this, tr("Save"), tr("The active document could not be saved."));
        return;
    }
    mStatusLabel->setText(tr("Active document saved."));
}
