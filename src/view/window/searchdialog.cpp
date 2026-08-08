#include "searchdialog.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/collection.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/tools/editrecordcommand.hpp"
#include "../../model/tools/undostack.hpp"
#include "../../model/tools/formcomponentsresolver.hpp"
#include "qtformdialogmanager.hpp"
#include "npceditor.hpp"
#include "weaponeditor.hpp"
#include "armor_editor.hpp"
#include "spell_editor.hpp"
#include "quest_editor.hpp"
#include "logger.hpp"

#include "../../../libs/files/esm/npcrecord.hpp"

#include "globvar_editor.hpp"
#include "facteditor.hpp"
#include "materialeditor.hpp"
#include "tree_editor.hpp"
#include "stat_editor.hpp"
#include "actieditor.hpp"
#include "misceditor.hpp"
#include "alch_editor.hpp"
#include "ingr_editor.hpp"
#include "book_editor.hpp"
#include "ench_editor.hpp"
#include "cont_editor.hpp"
#include "race_editor.hpp"
#include "perk_editor.hpp"
#include "magic_editor.hpp"
#include "pack_editor.hpp"
#include "lcrteditor.hpp"
#include "classeditor.hpp"
#include "cell_editor.hpp"
#include "worldspace_editor.hpp"
#include "location_editor.hpp"
#include "ref_editor.hpp"
#include "dialeditor.hpp"
#include "infoeditor.hpp"
#include "../../../libs/files/esm/glob.hpp"
#include "../../../libs/files/esm/packagerecord.hpp"
#include "../../../libs/files/esm/classrecord.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>
#include <QMenu>
#include <QInputDialog>
#include <QSettings>
#include <QRegularExpression>
#include <QScrollArea>
#include <QGroupBox>

SearchDialog::SearchDialog(Data* data, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mSearchEdit(nullptr),
      mFilterButton(nullptr),
      mTypeCombo(nullptr),
      mFieldCombo(nullptr),
      mResultsList(nullptr),
      mEditButton(nullptr),
      mCloneButton(nullptr),
      mDeleteButton(nullptr),
      mCloseButton(nullptr),
      mStatusLabel(nullptr),
      mRegexCheckBox(nullptr),
      mHistoryCombo(nullptr),
      mSaveSearchBtn(nullptr),
      mLoadSearchBtn(nullptr),
      mCriteriaLayout(nullptr),
      mAddCriterionBtn(nullptr),
      mLogicCombo(nullptr)
{
    LOG_INFO("SearchDialog created");
    setupUI();
    loadHistory();
    loadSavedSearches();
}

SearchDialog::~SearchDialog()
{
}

void SearchDialog::setupUI()
{
    setWindowTitle("Search");
    setMinimumSize(750, 580);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // History combo
    auto* historyLayout = new QHBoxLayout();
    historyLayout->addWidget(new QLabel("History:"));
    mHistoryCombo = new QComboBox();
    mHistoryCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    historyLayout->addWidget(mHistoryCombo, 1);
    mainLayout->addLayout(historyLayout);

    // Search row: text + regex checkbox + filter button
    auto* searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel("Search:"));
    mSearchEdit = new QLineEdit();
    mSearchEdit->setPlaceholderText("Enter search text...");
    searchLayout->addWidget(mSearchEdit, 1);
    mRegexCheckBox = new QCheckBox("Regex");
    searchLayout->addWidget(mRegexCheckBox);
    mFilterButton = new QPushButton("Filter");
    searchLayout->addWidget(mFilterButton);
    mainLayout->addLayout(searchLayout);

    // Filter row
    auto* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel("Record Type:"));
    mTypeCombo = new QComboBox();
    mTypeCombo->addItem("All", -1);
    mTypeCombo->addItem("NPC_", static_cast<int>(CkId::Type_Npc_));
    mTypeCombo->addItem("WEAP_", static_cast<int>(CkId::Type_Weap_));
    mTypeCombo->addItem("ARMOR_", static_cast<int>(CkId::Type_Armor_));
    mTypeCombo->addItem("SPEL_", static_cast<int>(CkId::Type_Spel_));
    mTypeCombo->addItem("MAGIC_", static_cast<int>(CkId::Type_Magic_));
    mTypeCombo->addItem("QUEST_", static_cast<int>(CkId::Type_Quest_));
    mTypeCombo->addItem("PACK_", static_cast<int>(CkId::Type_Pack_));
    mTypeCombo->addItem("ALCH_", static_cast<int>(CkId::Type_Alch_));
    mTypeCombo->addItem("INGR_", static_cast<int>(CkId::Type_Ingr_));
    mTypeCombo->addItem("CONT_", static_cast<int>(CkId::Type_Cont_));
    mTypeCombo->addItem("ENCH_", static_cast<int>(CkId::Type_Ench_));
    mTypeCombo->addItem("BOOK_", static_cast<int>(CkId::Type_Book_));
    mTypeCombo->addItem("MISC_", static_cast<int>(CkId::Type_Misc_));
    mTypeCombo->addItem("ACTI_", static_cast<int>(CkId::Type_Acti_));
    mTypeCombo->addItem("RACE_", static_cast<int>(CkId::Type_Race_));
    mTypeCombo->addItem("CLASS_", static_cast<int>(CkId::Type_Class_));
    mTypeCombo->addItem("FACT_", static_cast<int>(CkId::Type_Fact_));
    mTypeCombo->addItem("PERK_", static_cast<int>(CkId::Type_PerK_));
    mTypeCombo->addItem("CELL_", static_cast<int>(CkId::Type_Cel_));
    mTypeCombo->addItem("WRLD_", static_cast<int>(CkId::Type_WRLD_));
    mTypeCombo->addItem("LOCT_", static_cast<int>(CkId::Type_LOCT_));
    mTypeCombo->addItem("REFR_", static_cast<int>(CkId::Type_Refr_));
    mTypeCombo->addItem("DIAL_", static_cast<int>(CkId::Type_Dial_));
    mTypeCombo->addItem("INFO_", static_cast<int>(CkId::Type_Info_));
    filterLayout->addWidget(mTypeCombo);

    filterLayout->addWidget(new QLabel("Field:"));
    mFieldCombo = new QComboBox();
    mFieldCombo->addItem("All Fields", "");
    mFieldCombo->addItem("EditorID", "EditorID");
    mFieldCombo->addItem("FormID", "FormID");
    mFieldCombo->addItem("Name", "Name");
    filterLayout->addWidget(mFieldCombo);

    mainLayout->addLayout(filterLayout);

    // Multi-criteria section
    auto* criteriaGroup = new QGroupBox("Additional Criteria");
    auto* criteriaGroupLayout = new QVBoxLayout(criteriaGroup);
    auto* criteriaHeaderLayout = new QHBoxLayout();
    criteriaHeaderLayout->addWidget(new QLabel("Logic:"));
    mLogicCombo = new QComboBox();
    mLogicCombo->addItem("AND (all must match)", true);
    mLogicCombo->addItem("OR (any must match)", false);
    criteriaHeaderLayout->addWidget(mLogicCombo);
    criteriaHeaderLayout->addStretch();
    mAddCriterionBtn = new QPushButton("+ Add Criterion");
    criteriaHeaderLayout->addWidget(mAddCriterionBtn);
    criteriaGroupLayout->addLayout(criteriaHeaderLayout);

    mCriteriaLayout = new QVBoxLayout();
    criteriaGroupLayout->addLayout(mCriteriaLayout);
    criteriaGroupLayout->addStretch();
    mainLayout->addWidget(criteriaGroup);

    // Results list
    mResultsList = new QListWidget();
    mResultsList->setAlternatingRowColors(true);
    mResultsList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mResultsList->setContextMenuPolicy(Qt::CustomContextMenu);
    mainLayout->addWidget(mResultsList, 1);

    // Bottom button bar
    auto* buttonLayout = new QHBoxLayout();

    mSaveSearchBtn = new QPushButton("Save Search");
    buttonLayout->addWidget(mSaveSearchBtn);

    mLoadSearchBtn = new QPushButton("Load Search");
    buttonLayout->addWidget(mLoadSearchBtn);

    buttonLayout->addStretch();

    mEditButton = new QPushButton("Edit");
    mEditButton->setEnabled(false);
    buttonLayout->addWidget(mEditButton);

    mCloneButton = new QPushButton("Clone");
    mCloneButton->setEnabled(false);
    buttonLayout->addWidget(mCloneButton);

    mDeleteButton = new QPushButton("Delete");
    mDeleteButton->setEnabled(false);
    buttonLayout->addWidget(mDeleteButton);

    mCloseButton = new QPushButton("Close");
    buttonLayout->addWidget(mCloseButton);

    mainLayout->addLayout(buttonLayout);

    mStatusLabel = new QLabel("Ready");
    mainLayout->addWidget(mStatusLabel);

    // Connections
    connect(mFilterButton, &QPushButton::clicked, this, &SearchDialog::onSearch);
    connect(mSearchEdit, &QLineEdit::returnPressed, this, &SearchDialog::onSearch);
    connect(mTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { onSearch(); });
    connect(mFieldCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { onSearch(); });
    connect(mResultsList, &QListWidget::itemDoubleClicked, this, &SearchDialog::onDoubleClicked);
    connect(mHistoryCombo, QOverload<int>::of(&QComboBox::activated), this, &SearchDialog::onHistoryActivated);
    connect(mSaveSearchBtn, &QPushButton::clicked, this, &SearchDialog::onSaveSearch);
    connect(mLoadSearchBtn, &QPushButton::clicked, this, &SearchDialog::onLoadSearch);
    connect(mAddCriterionBtn, &QPushButton::clicked, this, &SearchDialog::onAddCriterion);

    connect(mEditButton, &QPushButton::clicked, this, [this]() {
        if (mResultsList->currentItem())
        {
            int row = mResultsList->row(mResultsList->currentItem());
            if (row >= 0 && row < mResults.size())
                openRecordEditor(mResults[row]);
        }
    });

    connect(mCloneButton, &QPushButton::clicked, this, &SearchDialog::cloneSelected);
    connect(mDeleteButton, &QPushButton::clicked, this, &SearchDialog::deleteSelected);
    connect(mCloseButton, &QPushButton::clicked, this, &QDialog::accept);

    connect(mResultsList, &QListWidget::currentRowChanged, this, [this](int row) {
        bool hasSelection = (row >= 0);
        mEditButton->setEnabled(hasSelection);
        mCloneButton->setEnabled(hasSelection);
        mDeleteButton->setEnabled(hasSelection);
    });

    connect(mResultsList, &QListWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QListWidgetItem* item = mResultsList->itemAt(pos);
        if (!item)
            return;

        int row = mResultsList->row(item);
        if (row < 0 || row >= mResults.size())
            return;

        QMenu contextMenu(this);
        QAction* copyFormIdAction = contextMenu.addAction("Copy FormID");

        QList<QListWidgetItem*> selected = mResultsList->selectedItems();
        if (selected.count() == 1)
        {
            QAction* editAction = contextMenu.addAction("Edit...");
            QAction* cloneAction = contextMenu.addAction("Clone...");
            QAction* deleteAction = contextMenu.addAction("Delete");

            QAction* chosen = contextMenu.exec(mResultsList->viewport()->mapToGlobal(pos));
            if (chosen == copyFormIdAction)
                onCopyFormId();
            else if (chosen == editAction)
                openRecordEditor(mResults[row]);
            else if (chosen == cloneAction)
                cloneSelected();
            else if (chosen == deleteAction)
                deleteSelected();
        }
        else if (selected.count() > 1)
        {
            QAction* batchSetEditorId = contextMenu.addAction(
                QString("Set EditorID for %1 records...").arg(selected.count()));
            QAction* batchDupeIds = contextMenu.addAction(
                QString("Clone %1 records...").arg(selected.count()));

            QAction* chosen = contextMenu.exec(mResultsList->viewport()->mapToGlobal(pos));
            if (chosen == copyFormIdAction)
                onCopyFormId();
            else if (chosen == batchSetEditorId)
                batchSetEditorIdForSelected();
            else if (chosen == batchDupeIds)
                batchCloneSelected();
        }
    });
}

void SearchDialog::onCopyFormId()
{
    QListWidgetItem* item = mResultsList->currentItem();
    if (!item)
        return;

    int row = mResultsList->row(item);
    if (row < 0 || row >= mResults.size())
        return;

    const auto& result = mResults[row];
    QApplication::clipboard()->setText(result.formId);
    mStatusLabel->setText(QString("Copied FormID: %1").arg(result.formId));
}

void SearchDialog::cloneSelected()
{
    QListWidgetItem* item = mResultsList->currentItem();
    if (!item)
        return;

    int row = mResultsList->row(item);
    if (row < 0 || row >= mResults.size())
        return;

    const auto& result = mResults[row];

    bool ok = false;
    QString newId = QInputDialog::getText(this, "Clone Record",
        QString("New Editor ID for clone of '%1':").arg(result.editorId),
        QLineEdit::Normal, result.editorId + "_clone", &ok);

    if (ok && !newId.isEmpty())
    {
        if (mData->cloneRecordWithUndo(result.type, result.editorId, newId))
        {
            LOG_INFO(QString("Record '%1' cloned to '%2'").arg(result.editorId, newId));
            mStatusLabel->setText(QString("Cloned '%1' to '%2'").arg(result.editorId, newId));
            onSearch();
        }
        else
        {
            QMessageBox::warning(this, "Clone Failed",
                QString("Could not clone '%1'.\n\nID '%2' may already exist.").arg(result.editorId, newId));
        }
    }
}

void SearchDialog::deleteSelected()
{
    QListWidgetItem* item = mResultsList->currentItem();
    if (!item)
        return;

    int row = mResultsList->row(item);
    if (row < 0 || row >= mResults.size())
        return;

    const auto& result = mResults[row];

    auto reply = QMessageBox::question(this, "Delete Record",
        QString("Are you sure you want to delete '%1'?\n\nThis action can be undone with Ctrl+Z.").arg(result.editorId),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        if (mData->removeRecord(result.type, result.editorId))
        {
            LOG_INFO(QString("Record '%1' deleted").arg(result.editorId));
            mStatusLabel->setText(QString("Deleted '%1'").arg(result.editorId));
            onSearch();
        }
        else
        {
            QMessageBox::warning(this, "Delete Failed",
                QString("Could not delete '%1'.").arg(result.editorId));
        }
    }
}

void SearchDialog::batchSetEditorIdForSelected()
{
    QList<QListWidgetItem*> selected = mResultsList->selectedItems();
    if (selected.isEmpty())
        return;

    bool ok = false;
    QString newEditorId = QInputDialog::getText(this,
        "Batch Set EditorID",
        QString("Enter new EditorID for %1 record(s):").arg(selected.count()),
        QLineEdit::Normal, "", &ok);

    if (!ok || newEditorId.isEmpty())
        return;

    QVector<QString> srcIds;
    CkId::Type type = CkId::Type_None;

    for (QListWidgetItem* item : selected)
    {
        int row = mResultsList->row(item);
        if (row < 0 || row >= mResults.size())
            continue;

        const auto& result = mResults[row];
        srcIds.append(result.editorId);
        type = result.type;
    }

    if (!srcIds.isEmpty())
    {
        mData->batchSetEditorIdWithUndo(type, srcIds, newEditorId);
    }

    onSearch();
}

void SearchDialog::batchCloneSelected()
{
    QList<QListWidgetItem*> selected = mResultsList->selectedItems();
    if (selected.isEmpty())
        return;

    QVector<QString> srcIds;
    QVector<QString> destIds;
    CkId::Type type = CkId::Type_None;

    int offset = 0;
    for (QListWidgetItem* item : selected)
    {
        int row = mResultsList->row(item);
        if (row < 0 || row >= mResults.size())
            continue;

        const auto& result = mResults[row];
        srcIds.append(result.editorId);
        destIds.append(result.editorId + "_clone" + QString::number(offset++));
        type = result.type;
    }

    if (!srcIds.isEmpty())
    {
        mData->batchCloneWithUndo(type, srcIds, destIds);
    }

    onSearch();
}

QList<int> SearchDialog::getSelectedIndices() const
{
    QList<int> indices;
    for (int i = 0; i < mResultsList->count(); ++i)
    {
        if (mResultsList->item(i)->isSelected())
            indices.append(i);
    }
    return indices;
}

bool SearchDialog::enableBatchEditing(bool enabled)
{
    mResultsList->setSelectionMode(enabled ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection);
    mEditButton->setEnabled(!enabled);
    mCloneButton->setEnabled(!enabled);
    mDeleteButton->setEnabled(!enabled);
    mStatusLabel->setText(enabled ? "Multi-select mode" : "Single-select mode");
    return true;
}

void SearchDialog::onSearch()
{
    QString text = mSearchEdit->text();
    QString field = mFieldCombo->currentData().toString();
    int typeIdx = mTypeCombo->currentData().toInt();
    CkId::Type typeFilter = (typeIdx == -1) ? CkId::Type_None : static_cast<CkId::Type>(typeIdx);

    if (text.isEmpty() && mCriteriaRows.isEmpty())
    {
        mResults.clear();
        populateResults(mResults);
        mStatusLabel->setText("Enter search text and click Filter");
        return;
    }

    // Validate regex if regex checkbox is checked
    if (mRegexCheckBox->isChecked() && !text.isEmpty())
    {
        QRegularExpression re(text);
        if (!re.isValid())
        {
            QMessageBox::warning(this, "Invalid Regex",
                QString("Invalid regular expression:\n%1").arg(re.errorString()));
            return;
        }
    }

    SearchAlgorithm::SearchCriteria criteria;
    criteria.text = text;
    criteria.field = field;
    criteria.typeFilter = typeFilter;
    criteria.matchMode = getCurrentMatchMode();
    criteria.caseSensitive = false;
    criteria.allCriteriaMustMatch = mLogicCombo->currentData().toBool();

    // Collect additional criteria from rows
    for (QWidget* rowWidget : mCriteriaRows)
    {
        auto* fieldCombo = rowWidget->findChild<QComboBox*>("criterionField");
        auto* modeCombo = rowWidget->findChild<QComboBox*>("criterionMode");
        auto* textEdit = rowWidget->findChild<QLineEdit*>("criterionText");

        if (!fieldCombo || !modeCombo || !textEdit)
            continue;

        QString critText = textEdit->text();
        if (critText.isEmpty())
            continue;

        // Validate regex in criterion mode
        if (modeCombo->currentData().toInt() == static_cast<int>(SearchAlgorithm::MatchMode::Regex))
        {
            QRegularExpression re(critText);
            if (!re.isValid())
            {
                QMessageBox::warning(this, "Invalid Regex",
                    QString("Invalid regex in criterion:\n%1").arg(re.errorString()));
                return;
            }
        }

        SearchAlgorithm::SearchCriteria::Criterion crit;
        crit.field = fieldCombo->currentData().toString();
        crit.value = critText;
        crit.mode = static_cast<SearchAlgorithm::MatchMode>(modeCombo->currentData().toInt());
        crit.caseSensitive = false;
        criteria.additionalCriteria.append(crit);
    }

    // Add to history
    if (!text.isEmpty())
        addToHistory(text);

    mResults = SearchAlgorithm::searchAdvanced(*mData, criteria);
    populateResults(mResults);

    LOG_INFO(QString("SearchDialog: Found %1 result(s)").arg(mResults.size()));
    mStatusLabel->setText(QString("Found %1 result(s)").arg(mResults.size()));
}

void SearchDialog::populateResults(const QVector<SearchAlgorithm::SearchResult>& results)
{
    mResultsList->clear();

    for (const auto& result : results)
    {
        QListWidgetItem* item = new QListWidgetItem(formatResultText(result));
        item->setData(Qt::UserRole, result.recordIndex);
        mResultsList->addItem(item);
    }
}

QString SearchDialog::formatResultText(const SearchAlgorithm::SearchResult& result) const
{
    return QString("[%1] %2 (FormID: %3)")
        .arg(CkId(result.type).getTypeName())
        .arg(result.editorId)
        .arg(result.formId);
}

void SearchDialog::onDoubleClicked(QListWidgetItem* item)
{
    if (!item)
        return;

    int row = mResultsList->row(item);
    if (row >= 0 && row < mResults.size())
        openRecordEditor(mResults[row]);
}

void SearchDialog::openRecordEditor(const SearchAlgorithm::SearchResult& result)
{
    switch (result.type)
    {
    case CkId::Type_Npc_:
    {
        auto& collection = mData->getNpcCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            NpcRecord originalState = collection.getRecord(result.recordIndex).get();
            NpcRecord editedState = originalState;
            NpcEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getNpcCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<NpcRecord>* cmd = new EditRecordCommand<NpcRecord>(&coll, idx, originalState, editedState,
                        "Edit NPC: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("NPC '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Weap_:
    {
        auto& collection = mData->getWeaponCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            WeaponRecord originalState = collection.getRecord(result.recordIndex).get();
            WeaponRecord editedState = originalState;
            WeaponEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getWeaponCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<WeaponRecord>* cmd = new EditRecordCommand<WeaponRecord>(&coll, idx, originalState, editedState,
                        "Edit Weapon: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Weapon '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Armor_:
    {
        auto& collection = mData->getArmorCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            ArmorRecord originalState = collection.getRecord(result.recordIndex).get();
            ArmorRecord editedState = originalState;
            ArmorEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getArmorCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<ArmorRecord>* cmd = new EditRecordCommand<ArmorRecord>(&coll, idx, originalState, editedState,
                        "Edit Armor: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Armor '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Spel_:
    {
        auto& collection = mData->getSpellCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            SpellRecord originalState = collection.getRecord(result.recordIndex).get();
            SpellRecord editedState = originalState;
            SpellEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getSpellCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<SpellRecord>* cmd = new EditRecordCommand<SpellRecord>(&coll, idx, originalState, editedState,
                        "Edit Spell: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Spell '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Quest_:
    {
        auto& collection = mData->getQuestCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            QuestRecord originalState = collection.getRecord(result.recordIndex).get();
            QuestRecord editedState = originalState;
            QuestEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getQuestCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<QuestRecord>* cmd = new EditRecordCommand<QuestRecord>(&coll, idx, originalState, editedState,
                        "Edit Quest: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Quest '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Glob_:
    {
        auto& collection = mData->getGlobCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            GlobalVariable originalState = collection.getRecord(result.recordIndex).get();
            GlobalVariable editedState = originalState;
            GlobVarEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getGlobCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<GlobalVariable>* cmd = new EditRecordCommand<GlobalVariable>(&coll, idx, originalState, editedState,
                        "Edit Global: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Global '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Tree_:
    {
        auto& collection = mData->getTreeCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            TreeRecord originalState = collection.getRecord(result.recordIndex).get();
            TreeRecord editedState = originalState;
            TreeEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getTreeCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<TreeRecord>* cmd = new EditRecordCommand<TreeRecord>(&coll, idx, originalState, editedState,
                        "Edit Tree: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Tree '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Stat_:
    {
        auto& collection = mData->getStatCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            StatRecord originalState = collection.getRecord(result.recordIndex).get();
            StatRecord editedState = originalState;
            StatEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getStatCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<StatRecord>* cmd = new EditRecordCommand<StatRecord>(&coll, idx, originalState, editedState,
                        "Edit Static: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Static '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Acti_:
    {
        auto& collection = mData->getActiCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            ActiRecord originalState = collection.getRecord(result.recordIndex).get();
            ActiRecord editedState = originalState;
            ActiEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getActiCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<ActiRecord>* cmd = new EditRecordCommand<ActiRecord>(&coll, idx, originalState, editedState,
                        "Edit Activator: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Activator '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Misc_:
    {
        auto& collection = mData->getMiscCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            MiscRecord originalState = collection.getRecord(result.recordIndex).get();
            MiscRecord editedState = originalState;
            MiscEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getMiscCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<MiscRecord>* cmd = new EditRecordCommand<MiscRecord>(&coll, idx, originalState, editedState,
                        "Edit Misc: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Misc '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Alch_:
    {
        auto& collection = mData->getAlchCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            AlchRecord originalState = collection.getRecord(result.recordIndex).get();
            AlchRecord editedState = originalState;
            AlchEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getAlchCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<AlchRecord>* cmd = new EditRecordCommand<AlchRecord>(&coll, idx, originalState, editedState,
                        "Edit Potion: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Potion '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Ingr_:
    {
        auto& collection = mData->getIngrCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            IngrRecord originalState = collection.getRecord(result.recordIndex).get();
            IngrRecord editedState = originalState;
            IngrEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getIngrCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<IngrRecord>* cmd = new EditRecordCommand<IngrRecord>(&coll, idx, originalState, editedState,
                        "Edit Ingredient: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Ingredient '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Book_:
    {
        auto& collection = mData->getBookCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            BookRecord originalState = collection.getRecord(result.recordIndex).get();
            BookRecord editedState = originalState;
            BookEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getBookCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<BookRecord>* cmd = new EditRecordCommand<BookRecord>(&coll, idx, originalState, editedState,
                        "Edit Book: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Book '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Ench_:
    {
        auto& collection = mData->getEnchCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            EnchRecord originalState = collection.getRecord(result.recordIndex).get();
            EnchRecord editedState = originalState;
            EnchEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getEnchCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<EnchRecord>* cmd = new EditRecordCommand<EnchRecord>(&coll, idx, originalState, editedState,
                        "Edit Enchantment: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Enchantment '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Cont_:
    {
        auto& collection = mData->getContCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            ContRecord originalState = collection.getRecord(result.recordIndex).get();
            ContRecord editedState = originalState;
            ContEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getContCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<ContRecord>* cmd = new EditRecordCommand<ContRecord>(&coll, idx, originalState, editedState,
                        "Edit Container: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Container '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Race_:
    {
        auto& collection = mData->getRaceCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            RaceRecord originalState = collection.getRecord(result.recordIndex).get();
            RaceRecord editedState = originalState;
            RaceEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getRaceCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<RaceRecord>* cmd = new EditRecordCommand<RaceRecord>(&coll, idx, originalState, editedState,
                        "Edit Race: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Race '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_PerK_:
    {
        auto& collection = mData->getPerkCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            PerkRecord originalState = collection.getRecord(result.recordIndex).get();
            PerkRecord editedState = originalState;
            PerkEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getPerkCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<PerkRecord>* cmd = new EditRecordCommand<PerkRecord>(&coll, idx, originalState, editedState,
                        "Edit Perk: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Perk '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Magic_:
    {
        auto& collection = mData->getMagicCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            MagicRecord originalState = collection.getRecord(result.recordIndex).get();
            MagicRecord editedState = originalState;
            MagicEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getMagicCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<MagicRecord>* cmd = new EditRecordCommand<MagicRecord>(&coll, idx, originalState, editedState,
                        "Edit Magic: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Magic '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Pack_:
    {
        auto& collection = mData->getPackCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            PackageRecord originalState = collection.getRecord(result.recordIndex).get();
            PackageRecord editedState = originalState;
            PackEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getPackCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<PackageRecord>* cmd = new EditRecordCommand<PackageRecord>(&coll, idx, originalState, editedState,
                        "Edit Package: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Package '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Lcrt_:
    {
        auto& collection = mData->getLcrtCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            LocationRefType originalState = collection.getRecord(result.recordIndex).get();
            LocationRefType editedState = originalState;
            LcrtEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getLcrtCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<LocationRefType>* cmd = new EditRecordCommand<LocationRefType>(&coll, idx, originalState, editedState,
                        "Edit LCRT: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("LCRT '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Class_:
    {
        auto& collection = mData->getClassCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            ClassRecord originalState = collection.getRecord(result.recordIndex).get();
            ClassRecord editedState = originalState;
            ClassEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getClassCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<ClassRecord>* cmd = new EditRecordCommand<ClassRecord>(&coll, idx, originalState, editedState,
                        "Edit Class: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Class '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Cel_:
    {
        auto& collection = mData->getCellCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            CellRecord originalState = collection.getRecord(result.recordIndex).get();
            CellRecord editedState = originalState;
            CellEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getCellCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<CellRecord>* cmd = new EditRecordCommand<CellRecord>(&coll, idx, originalState, editedState,
                        "Edit Cell: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Cell '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_WRLD_:
    {
        auto& collection = mData->getWorldspaceCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            WorldspaceRecord originalState = collection.getRecord(result.recordIndex).get();
            WorldspaceRecord editedState = originalState;
            WorldspaceEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getWorldspaceCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<WorldspaceRecord>* cmd = new EditRecordCommand<WorldspaceRecord>(&coll, idx, originalState, editedState,
                        "Edit Worldspace: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Worldspace '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_LOCT_:
    {
        auto& collection = mData->getLocationCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            LocationRecord originalState = collection.getRecord(result.recordIndex).get();
            LocationRecord editedState = originalState;
            LocationEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getLocationCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<LocationRecord>* cmd = new EditRecordCommand<LocationRecord>(&coll, idx, originalState, editedState,
                        "Edit Location: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Location '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Refr_:
    {
        auto& collection = mData->getRefrCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            RefrRecord originalState = collection.getRecord(result.recordIndex).get();
            RefrRecord editedState = originalState;
            RefEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getRefrCollection();
                int idx = result.recordIndex;
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<RefrRecord>* cmd = new EditRecordCommand<RefrRecord>(&coll, idx, originalState, editedState,
                        "Edit Reference: 0x" + QString::number(editedState.formId, 16).toUpper().rightJustified(8, '0'));
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Reference '0x%1' edited via search").arg(editedState.formId, 8, 16, QChar('0')).toUpper());
            }
        }
        break;
    }
    case CkId::Type_Fact_:
    {
        auto& collection = mData->getFactCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            FactRecord originalState = collection.getRecord(result.recordIndex).get();
            FactRecord editedState = originalState;
            FactEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getFactCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<FactRecord>* cmd = new EditRecordCommand<FactRecord>(&coll, idx, originalState, editedState,
                        "Edit Faction: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Faction '%1' edited via search").arg(editedState.editorId));
            }
        }
        break;
    }
    case CkId::Type_Material_:
    {
        auto& collection = mData->getMaterialCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            MaterialRecord originalState = collection.getRecord(result.recordIndex).get();
            MaterialRecord editedState = originalState;
            MaterialEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getMaterialCollection();
                int idx = coll.searchId(editedState.name);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<MaterialRecord>* cmd = new EditRecordCommand<MaterialRecord>(&coll, idx, originalState, editedState,
                        "Edit Material: " + editedState.name);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Material '%1' edited via search").arg(editedState.name));
            }
        }
        break;
    }
    case CkId::Type_Dial_:
    {
        auto& collection = mData->getDialCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            DialRecord* dial = &collection.getRecord(result.recordIndex).get();
            DialRecord originalState = collection.getRecord(result.recordIndex).get();
            DialEditor editor(mData, this);
            editor.loadRecord(dial);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getDialCollection();
                int idx = coll.searchId(dial->editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<DialRecord>* cmd = new EditRecordCommand<DialRecord>(&coll, idx, originalState, *dial,
                        "Edit Dialogue: " + dial->editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Dialogue '%1' edited via search").arg(dial->editorId));
            }
        }
        break;
    }
    case CkId::Type_Info_:
    {
        auto& collection = mData->getInfoCollection();
        if (result.recordIndex >= 0 && result.recordIndex < collection.size())
        {
            InfoRecord* info = &collection.getRecord(result.recordIndex).get();
            InfoRecord originalState = collection.getRecord(result.recordIndex).get();
            InfoEditor editor(mData, this);
            editor.loadRecord(info);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getInfoCollection();
                int idx = coll.searchId(info->editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<InfoRecord>* cmd = new EditRecordCommand<InfoRecord>(&coll, idx, originalState, *info,
                        "Edit Dialogue Info: " + info->editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Dialogue Info '%1' edited via search").arg(info->editorId));
            }
        }
        break;
    }
    default:
    {
        // Record kinds that have no form editor: GameSetting (grid editor
        // only) and the internal log pseudo-types.
        static const QVector<CkId::Type> uneditableTypes = {
            CkId::Type_Gmst,
            CkId::Type_LoadingLog,
            CkId::Type_RunLog,
        };

        if (result.type != CkId::Type_None)
        {
            BaseCollection* coll = mData->getCollectionByType(result.type);
            if (coll && result.recordIndex >= 0 && result.recordIndex < coll->size())
            {
                openck::FormComponents* comps = nullptr;
                void* recPtr = nullptr;
                if (resolveComponents(coll, result.recordIndex, comps, recPtr) && comps)
                {
                    quint32 formId = coll->getFormId(result.recordIndex);
                    QString formIdKey = formId != 0
                        ? QStringLiteral("0x%1").arg(formId, 8, 16, QChar('0'))
                        : result.editorId;
                    openck::QtFormDialogManager::instance().openOrFocus(formIdKey, comps, this);
                    break;
                }
            }
            if (uneditableTypes.contains(result.type))
            {
                QMessageBox::information(this, "Edit",
                    QString("Editing %1 records is not supported from the search dialog yet "
                            "(see docs/REMAINING_WORK_PLAN.md Phase D).")
                        .arg(CkId(result.type).getTypeName()));
                break;
            }
        }
        break;
    }
    }
}

// --- Regex support ---

SearchAlgorithm::MatchMode SearchDialog::getCurrentMatchMode() const
{
    if (mRegexCheckBox->isChecked())
        return SearchAlgorithm::MatchMode::Regex;

    return SearchAlgorithm::MatchMode::Contains;
}

// --- Search history ---

void SearchDialog::loadHistory()
{
    QSettings settings("OpenCK", "SearchDialog");
    mSearchHistory = settings.value("SearchHistory").toStringList();
    updateHistoryCombo();
}

void SearchDialog::saveHistory()
{
    QSettings settings("OpenCK", "SearchDialog");
    settings.setValue("SearchHistory", mSearchHistory);
}

void SearchDialog::addToHistory(const QString& text)
{
    mSearchHistory.removeAll(text);
    mSearchHistory.prepend(text);
    while (mSearchHistory.size() > 20)
        mSearchHistory.removeLast();
    saveHistory();
    updateHistoryCombo();
}

void SearchDialog::updateHistoryCombo()
{
    mHistoryCombo->clear();
    mHistoryCombo->addItems(mSearchHistory);
}

void SearchDialog::onHistoryActivated(int index)
{
    if (index >= 0 && index < mSearchHistory.size())
    {
        mSearchEdit->setText(mSearchHistory[index]);
        onSearch();
    }
}

// --- Saved searches ---

void SearchDialog::loadSavedSearches()
{
    // Saved searches are loaded on demand via onLoadSearch
}

QVariantMap SearchDialog::buildCurrentCriteriaMap() const
{
    QVariantMap map;
    map["text"] = mSearchEdit->text();
    map["field"] = mFieldCombo->currentData().toString();
    map["typeIndex"] = mTypeCombo->currentIndex();
    map["regexEnabled"] = mRegexCheckBox->isChecked();
    map["logicAnd"] = mLogicCombo->currentData().toBool();

    QVariantList criteriaList;
    for (QWidget* rowWidget : mCriteriaRows)
    {
        auto* fieldCombo = rowWidget->findChild<QComboBox*>("criterionField");
        auto* modeCombo = rowWidget->findChild<QComboBox*>("criterionMode");
        auto* textEdit = rowWidget->findChild<QLineEdit*>("criterionText");

        if (!fieldCombo || !modeCombo || !textEdit)
            continue;

        QVariantMap critMap;
        critMap["field"] = fieldCombo->currentData().toString();
        critMap["mode"] = modeCombo->currentData().toInt();
        critMap["text"] = textEdit->text();
        criteriaList.append(critMap);
    }
    map["criteria"] = criteriaList;

    return map;
}

void SearchDialog::applyCriteriaMap(const QVariantMap& map)
{
    if (map.contains("text"))
        mSearchEdit->setText(map["text"].toString());

    if (map.contains("field"))
    {
        QString field = map["field"].toString();
        for (int i = 0; i < mFieldCombo->count(); ++i)
        {
            if (mFieldCombo->itemData(i).toString() == field)
            {
                mFieldCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    if (map.contains("typeIndex"))
        mTypeCombo->setCurrentIndex(map["typeIndex"].toInt());

    if (map.contains("regexEnabled"))
        mRegexCheckBox->setChecked(map["regexEnabled"].toBool());

    if (map.contains("logicAnd"))
    {
        bool isAnd = map["logicAnd"].toBool();
        mLogicCombo->setCurrentIndex(isAnd ? 0 : 1);
    }

    // Clear existing criteria
    for (QWidget* row : mCriteriaRows)
        row->deleteLater();
    mCriteriaRows.clear();

    // Rebuild criteria
    if (map.contains("criteria"))
    {
        QVariantList criteriaList = map["criteria"].toList();
        for (const QVariant& critVar : criteriaList)
        {
            QVariantMap critMap = critVar.toMap();
            QString field = critMap["field"].toString();
            int mode = critMap["mode"].toInt();
            QString text = critMap["text"].toString();

            QStringList modeNames = {"Contains", "StartsWith", "EndsWith", "Exact", "Regex"};
            QString modeName = (mode >= 0 && mode < modeNames.size()) ? modeNames[mode] : "Contains";
            addCriterionRow(field, modeName, text);
        }
    }

    onSearch();
}

void SearchDialog::onSaveSearch()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, "Save Search",
        "Name for this search:", QLineEdit::Normal, "", &ok);

    if (!ok || name.isEmpty())
        return;

    QSettings settings("OpenCK", "SearchDialog");
    QVariantMap savedSearches = settings.value("SavedSearches").toMap();
    savedSearches[name] = buildCurrentCriteriaMap();
    settings.setValue("SavedSearches", savedSearches);

    mStatusLabel->setText(QString("Search saved as '%1'").arg(name));
}

void SearchDialog::onLoadSearch()
{
    QSettings settings("OpenCK", "SearchDialog");
    QVariantMap savedSearches = settings.value("SavedSearches").toMap();

    if (savedSearches.isEmpty())
    {
        QMessageBox::information(this, "Load Search", "No saved searches found.");
        return;
    }

    QStringList names = savedSearches.keys();
    bool ok = false;
    QString selected = QInputDialog::getItem(this, "Load Search",
        "Select a saved search:", names, 0, false, &ok);

    if (!ok || selected.isEmpty())
        return;

    QVariantMap criteriaMap = savedSearches[selected].toMap();
    applyCriteriaMap(criteriaMap);
}

// --- Multi-criteria ---

void SearchDialog::onAddCriterion()
{
    addCriterionRow();
}

QWidget* SearchDialog::createCriterionRow(int index)
{
    auto* row = new QWidget();
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* fieldCombo = new QComboBox();
    fieldCombo->setObjectName("criterionField");
    fieldCombo->addItem("All Fields", "");
    fieldCombo->addItem("EditorID", "EditorID");
    fieldCombo->addItem("FormID", "FormID");
    fieldCombo->addItem("Name", "Name");
    layout->addWidget(fieldCombo);

    auto* modeCombo = new QComboBox();
    modeCombo->setObjectName("criterionMode");
    modeCombo->addItem("Contains", static_cast<int>(SearchAlgorithm::MatchMode::Contains));
    modeCombo->addItem("StartsWith", static_cast<int>(SearchAlgorithm::MatchMode::StartsWith));
    modeCombo->addItem("EndsWith", static_cast<int>(SearchAlgorithm::MatchMode::EndsWith));
    modeCombo->addItem("Exact", static_cast<int>(SearchAlgorithm::MatchMode::Exact));
    modeCombo->addItem("Regex", static_cast<int>(SearchAlgorithm::MatchMode::Regex));
    layout->addWidget(modeCombo);

    auto* textEdit = new QLineEdit();
    textEdit->setObjectName("criterionText");
    textEdit->setPlaceholderText("Value...");
    layout->addWidget(textEdit, 1);

    auto* removeBtn = new QPushButton("-");
    removeBtn->setMaximumWidth(30);
    layout->addWidget(removeBtn);

    int capturedIndex = index;
    connect(removeBtn, &QPushButton::clicked, this, [this, capturedIndex]() {
        onRemoveCriterion(capturedIndex);
    });

    return row;
}

void SearchDialog::addCriterionRow(const QString& field, const QString& mode, const QString& text)
{
    int index = mCriteriaRows.size();
    QWidget* row = createCriterionRow(index);
    mCriteriaRows.append(row);
    mCriteriaLayout->addWidget(row);

    // Set initial values
    auto* fieldCombo = row->findChild<QComboBox*>("criterionField");
    auto* modeCombo = row->findChild<QComboBox*>("criterionMode");
    auto* textEdit = row->findChild<QLineEdit*>("criterionText");

    if (fieldCombo && !field.isEmpty())
    {
        for (int i = 0; i < fieldCombo->count(); ++i)
        {
            if (fieldCombo->itemData(i).toString() == field)
            {
                fieldCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    if (modeCombo && !mode.isEmpty())
    {
        for (int i = 0; i < modeCombo->count(); ++i)
        {
            if (modeCombo->itemText(i) == mode)
            {
                modeCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    if (textEdit && !text.isEmpty())
        textEdit->setText(text);
}

void SearchDialog::onRemoveCriterion(int index)
{
    if (index >= 0 && index < mCriteriaRows.size())
    {
        QWidget* row = mCriteriaRows.takeAt(index);
        mCriteriaLayout->removeWidget(row);
        row->deleteLater();
        rebuildCriteriaLayout();
    }
}

void SearchDialog::rebuildCriteriaLayout()
{
    // Re-index all remove buttons after a deletion
    for (int i = 0; i < mCriteriaRows.size(); ++i)
    {
        QWidget* row = mCriteriaRows[i];
        if (auto* removeBtn = row->findChild<QPushButton*>())
        {
            disconnect(removeBtn, nullptr, this, nullptr);
            connect(removeBtn, &QPushButton::clicked, this, [this, i]() {
                onRemoveCriterion(i);
            });
        }
    }
}