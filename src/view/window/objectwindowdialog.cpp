#include "objectwindowdialog.hpp"

#include "../../model/window/objectwindow.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/collection.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/tools/editrecordcommand.hpp"
#include "../../model/tools/undostack.hpp"
#include "../../view/messageboxhelper.hpp"
#include "qtformdialogmanager.hpp"
#include "npceditor.hpp"
#include "weaponeditor.hpp"
#include "armor_editor.hpp"
#include "spell_editor.hpp"
#include "quest_editor.hpp"
#include "globvar_editor.hpp"
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
#include "materialeditor.hpp"
#include "facteditor.hpp"
#include "logger.hpp"
#include "../../model/tools/blenderlauncher.hpp"
#include "nifpreviewdialog.hpp"
#include "nifcomparisondialog.hpp"
#include "../../../libs/files/esm/glob.hpp"
#include "../../../libs/files/esm/packagerecord.hpp"
#include "../../../libs/files/esm/classrecord.hpp"
#include "../../../libs/files/esm/factrecord.hpp"
#include "../../../libs/files/esm/perkrecord.hpp"
#include "../../../libs/files/esm/cellrecord.hpp"
#include "../../../libs/files/esm/worldspacerecord.hpp"
#include "../../../libs/files/esm/locationrecord.hpp"
#include "../../../libs/files/esm/refrecord.hpp"
#include "../../../libs/files/esm/materialrecord.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>

ObjectWindowDialog::ObjectWindowDialog(Data* data, QWidget* parent)
    : QDockWidget(parent),
      mData(data),
      mModel(nullptr),
      mTreeView(nullptr),
      mFilterEdit(nullptr),
      mEditButton(nullptr),
      mDeleteButton(nullptr),
      mCloneButton(nullptr),
      mStatusLabel(nullptr),
      mContextMenu(nullptr)
{
    setWindowTitle("Object Window");
    setupUI();
}

ObjectWindowDialog::~ObjectWindowDialog()
{
}

void ObjectWindowDialog::setupUI()
{
    setMinimumSize(800, 600);

    auto* centralWidget = new QWidget();
    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    auto* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel("Filter:"));
    mFilterEdit = new QLineEdit();
    mFilterEdit->setPlaceholderText("Search by Editor ID or Form ID...");
    filterLayout->addWidget(mFilterEdit, 1);
    mainLayout->addLayout(filterLayout);

    mTreeView = new QTreeView();
    mTreeView->setAlternatingRowColors(true);
    mTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mTreeView->setRootIsDecorated(true);
    mTreeView->setContextMenuPolicy(Qt::CustomContextMenu);

    mModel = new ObjectWindowModel(this);
    mModel->setData(mData);
    mTreeView->setModel(mModel);

    mTreeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    mTreeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    mTreeView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    mainLayout->addWidget(mTreeView, 1);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    mEditButton = new QPushButton("Edit...");
    mEditButton->setToolTip("Edit the selected record");
    buttonLayout->addWidget(mEditButton);

    mCloneButton = new QPushButton("Clone");
    mCloneButton->setToolTip("Clone the selected record");
    buttonLayout->addWidget(mCloneButton);

    mDeleteButton = new QPushButton("Delete");
    mDeleteButton->setToolTip("Delete the selected record");
    buttonLayout->addWidget(mDeleteButton);

    mainLayout->addLayout(buttonLayout);

    mStatusLabel = new QLabel();
    mainLayout->addWidget(mStatusLabel);

    connect(mFilterEdit, &QLineEdit::textChanged, mModel, &ObjectWindowModel::applyFilter);
    connect(mEditButton, &QPushButton::clicked, this, &ObjectWindowDialog::editSelected);
    connect(mDeleteButton, &QPushButton::clicked, this, &ObjectWindowDialog::deleteSelected);
    connect(mCloneButton, &QPushButton::clicked, this, &ObjectWindowDialog::cloneSelected);
    connect(mTreeView, &QTreeView::doubleClicked, this, &ObjectWindowDialog::onDoubleClick);
    connect(mTreeView, &QTreeView::customContextMenuRequested, this, [this](const QPoint& pos) {
        QModelIndex index = mTreeView->indexAt(pos);
        updateContextMenu(index);
        mContextMenu->exec(mTreeView->viewport()->mapToGlobal(pos));
    });

    updateContextMenu(QModelIndex());

    setWidget(centralWidget);
}

void ObjectWindowDialog::updateContextMenu(const QModelIndex& index)
{
    if (mContextMenu)
    {
        delete mContextMenu;
    }

    mContextMenu = new QMenu();

    if (!index.isValid() || mModel->parent(index).isValid())
    {
        QList<QModelIndex> selectedIndices = getSelectedIndices();
        int count = selectedIndices.count();

        if (count <= 1)
        {
            QAction* editAction = mContextMenu->addAction("Edit...");
            QAction* cloneAction = mContextMenu->addAction("Clone");
            QAction* deleteAction = mContextMenu->addAction("Delete");

            connect(editAction, &QAction::triggered, this, &ObjectWindowDialog::editSelected);
            connect(cloneAction, &QAction::triggered, this, &ObjectWindowDialog::cloneSelected);
            connect(deleteAction, &QAction::triggered, this, &ObjectWindowDialog::deleteSelected);

            // Add "Open in Blender" for records with 3D models
            int categoryId = getSelectedCategoryId(index);
            CkId::Type type = static_cast<CkId::Type>(mModel->getCategoryType(categoryId));
            
            bool hasModel = (type == CkId::Type_Weap_ || type == CkId::Type_Armor_ || 
                           type == CkId::Type_Stat_ || type == CkId::Type_Tree_ ||
                           type == CkId::Type_Acti_ || type == CkId::Type_Misc_ ||
                           type == CkId::Type_Book_ || type == CkId::Type_Ingr_ ||
                           type == CkId::Type_Alch_ || type == CkId::Type_Cont_);
            
            if (hasModel && BlenderLauncher::isBlenderAvailable()) {
                QAction* blenderAction = mContextMenu->addAction("Open in Blender...");
                connect(blenderAction, &QAction::triggered, this, &ObjectWindowDialog::openInBlender);

                QAction* previewAction = mContextMenu->addAction("Preview NIF...");
                connect(previewAction, &QAction::triggered, this, &ObjectWindowDialog::previewNif);

                QAction* compareAction = mContextMenu->addAction("Compare NIFs...");
                connect(compareAction, &QAction::triggered, this, &ObjectWindowDialog::compareNifs);
            }
        }

        if (count > 1)
        {
            QAction* batchSetIdAction = mContextMenu->addAction(
                QString("Set EditorID for %1 records...").arg(count));
            QAction* batchDuplicateAction = mContextMenu->addAction(
                QString("Duplicate %1 records...").arg(count));

            connect(batchSetIdAction, &QAction::triggered, this, &ObjectWindowDialog::batchSetEditorId);
            connect(batchDuplicateAction, &QAction::triggered, this, &ObjectWindowDialog::batchDuplicateIds);
        }
    }
    else
    {
        QAction* collapseAllAction = mContextMenu->addAction("Collapse All");
        QAction* expandAllAction = mContextMenu->addAction("Expand All");

        connect(collapseAllAction, &QAction::triggered, mTreeView, &QTreeView::collapseAll);
        connect(expandAllAction, &QAction::triggered, mTreeView, &QTreeView::expandAll);
    }
}
void ObjectWindowDialog::editSelected()
{
    QModelIndex index = mTreeView->currentIndex();
    if (!index.isValid() || !mModel->parent(index).isValid())
        return;

    int categoryId = mModel->getCategoryIndex(index);
    int recordIndex = mModel->getRecordIndex(index);
    QString editorId = mModel->getRecordEditorId(categoryId, recordIndex);
    CkId::Type type = static_cast<CkId::Type>(mModel->getCategoryType(categoryId));

    switch (type)
    {
    case CkId::Type_Npc_:
    {
        auto& collection = mData->getNpcCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            NpcRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &rec.components, this);
        }
        break;
    }
    case CkId::Type_Weap_:
    {
        auto& collection = mData->getWeaponCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            WeaponRecord& weap = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(weap.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &weap.components, this);
        }
        break;
    }
    case CkId::Type_Armor_:
    {
        auto& collection = mData->getArmorCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            ArmorRecord& armor = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(armor.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &armor.components, this);
        }
        break;
    }
    case CkId::Type_Spel_:
    {
        auto& collection = mData->getSpellCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            SpellRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &rec.components, this);
        }
        break;
    }
    case CkId::Type_Quest_:
    {
        auto& collection = mData->getQuestCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            QuestRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &rec.components, this);
        }
        break;
    }
    case CkId::Type_Glob_:
    {
        auto& collection = mData->getGlobCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            GlobalVariable originalState = collection.getRecord(recordIndex).get();
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
                LOG_INFO(QString("Global '%1' edited").arg(editorId));
            }
        }
        break;
    }
    case CkId::Type_Tree_:
    {
        auto& collection = mData->getTreeCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            TreeRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &rec.components, this);
        }
        break;
    }
    case CkId::Type_Stat_:
    {
        auto& collection = mData->getStatCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            StatRecord& stat = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(stat.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &stat.components, this);
        }
        break;
    }
    case CkId::Type_Acti_:
    {
        auto& collection = mData->getActiCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            ActiRecord& acti = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(acti.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &acti.components, this);
        }
        break;
    }
    case CkId::Type_Misc_:
    {
        auto& collection = mData->getMiscCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            MiscRecord& misc = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(misc.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &misc.components, this);
        }
        break;
    }
    case CkId::Type_Alch_:
    {
        auto& collection = mData->getAlchCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            AlchRecord& alch = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(alch.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &alch.components, this);
        }
        break;
    }
    case CkId::Type_Ingr_:
    {
        auto& collection = mData->getIngrCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            IngrRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &rec.components, this);
        }
        break;
    }
    case CkId::Type_Book_:
    {
        auto& collection = mData->getBookCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            BookRecord& book = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(book.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &book.components, this);
        }
        break;
    }
    case CkId::Type_Ench_:
    {
        auto& collection = mData->getEnchCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            EnchRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &rec.components, this);
        }
        break;
    }
    case CkId::Type_Cont_:
    {
        auto& collection = mData->getContCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            ContRecord& cont = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(cont.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &cont.components, this);
        }
        break;
    }
    case CkId::Type_Race_:
    {
        auto& collection = mData->getRaceCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            RaceRecord originalState = collection.getRecord(recordIndex).get();
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
                LOG_INFO(QString("Race '%1' edited").arg(editorId));
            }
        }
        break;
    }
    case CkId::Type_PerK_:
    {
        auto& collection = mData->getPerkCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            PerkRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &rec.components, this);
        }
        break;
    }
    case CkId::Type_Magic_:
    {
        auto& collection = mData->getMagicCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            MagicRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &rec.components, this);
        }
        break;
    }
    case CkId::Type_Pack_:
    {
        auto& collection = mData->getPackCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            PackageRecord originalState = collection.getRecord(recordIndex).get();
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
                LOG_INFO(QString("Package '%1' edited").arg(editorId));
            }
        }
        break;
    }
    case CkId::Type_Lcrt_:
    {
        auto& collection = mData->getLcrtCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            LocationRefType originalState = collection.getRecord(recordIndex).get();
            LocationRefType editedState = originalState;
            LcrtEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getLcrtCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<LocationRefType>* cmd = new EditRecordCommand<LocationRefType>(&coll, idx, originalState, editedState,
                        "Edit LocationRef: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("LocationRef '%1' edited").arg(editorId));
            }
        }
        break;
    }
    case CkId::Type_Class_:
    {
        auto& collection = mData->getClassCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            ClassRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &rec.components, this);
        }
        break;
    }
    case CkId::Type_Cel_:
    {
        auto& collection = mData->getCellCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            CellRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &rec.components, this);
        }
        break;
    }
    case CkId::Type_WRLD_:
    {
        auto& collection = mData->getWorldspaceCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            WorldspaceRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &rec.components, this);
        }
        break;
    }
    case CkId::Type_LOCT_:
    {
        auto& collection = mData->getLocationCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            LocationRecord originalState = collection.getRecord(recordIndex).get();
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
                LOG_INFO(QString("Location '%1' edited").arg(editorId));
            }
        }
        break;
    }
    case CkId::Type_Refr_:
    {
        auto& collection = mData->getRefrCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            RefrRecord originalState = collection.getRecord(recordIndex).get();
            RefrRecord editedState = originalState;
            RefEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getRefrCollection();
                int idx = recordIndex;
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<RefrRecord>* cmd = new EditRecordCommand<RefrRecord>(&coll, idx, originalState, editedState,
                        "Edit Reference: 0x" + QString::number(editedState.formId, 16).toUpper().rightJustified(8, '0'));
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Reference '0x%1' edited").arg(editedState.formId, 8, 16, QChar('0')).toUpper());
            }
        }
        break;
    }
    case CkId::Type_Dial_:
    {
        auto& collection = mData->getDialCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            DialRecord originalState = collection.getRecord(recordIndex).get();
            DialRecord editedState = originalState;
            DialEditor editor(mData, this);
            editor.loadRecord(&editedState);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getDialCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<DialRecord>* cmd = new EditRecordCommand<DialRecord>(&coll, idx, originalState, editedState,
                        "Edit Dialogue: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Dialogue '%1' edited").arg(editorId));
            }
        }
        break;
    }
    case CkId::Type_Info_:
    {
        auto& collection = mData->getInfoCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            InfoRecord originalState = collection.getRecord(recordIndex).get();
            InfoRecord editedState = originalState;
            InfoEditor editor(mData, this);
            editor.loadRecord(&editedState);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getInfoCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<InfoRecord>* cmd = new EditRecordCommand<InfoRecord>(&coll, idx, originalState, editedState,
                        "Edit Dialogue Info: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Dialogue Info '%1' edited").arg(editorId));
            }
        }
        break;
    }
    case CkId::Type_Fact_:
    {
        auto& collection = mData->getFactCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            FactRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &rec.components, this);
        }
        break;
    }
    case CkId::Type_Material_:
    {
        auto& collection = mData->getMaterialCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            MaterialRecord originalState = collection.getRecord(recordIndex).get();
            MaterialRecord editedState = originalState;
            MaterialEditor editor(mData, &editedState, this);
            if (editor.exec() == QDialog::Accepted)
            {
                auto& coll = mData->getMaterialCollection();
                int idx = coll.searchId(editedState.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<MaterialRecord>* cmd = new EditRecordCommand<MaterialRecord>(&coll, idx, originalState, editedState,
                        "Edit Material: " + editedState.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Material '%1' edited").arg(editorId));
            }
        }
        break;
    }
    default:
    {
        QMessageBox::information(this, "Edit Record",
            QString("Editing %1 record '%2'\n\n"
                    "This record type is not yet editable.\n"
                    "Supported types: NPC_, WEAP_, ARMOR_, SPEL_, QUEST_, GLOB_, TREE_, STAT_, ACTI_, MISC_, ALCH_, INGR_, BOOK_, ENCH_, CONT_, RACE_, PERK_, MAGIC_, PACK_, LCRT_, CLASS_, CELL_, WRLD_, LOCT_, REFR_, DIAL_, INFO_, FACT_, MATE_")
                .arg(CkId(type).getTypeName(), editorId));
        break;
    }
    }
}
void ObjectWindowDialog::deleteSelected()
{
    QModelIndex index = mTreeView->currentIndex();
    if (!index.isValid() || !mModel->parent(index).isValid())
        return;

    int categoryId = mModel->getCategoryIndex(index);
    int recordIndex = mModel->getRecordIndex(index);
    QString editorId = mModel->getRecordEditorId(categoryId, recordIndex);
    CkId::Type type = static_cast<CkId::Type>(mModel->getCategoryType(categoryId));

    auto result = QMessageBox::question(this, "Delete Record",
        QString("Are you sure you want to delete '%1'?\n\nThis action can be undone with Ctrl+Z.").arg(editorId),
        QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes)
    {
        if (mData->removeRecord(type, editorId))
        {
            LOG_INFO(QString("Record '%1' deleted").arg(editorId));
            mModel->setData(mData);
        }
        else
        {
            QMessageBox::warning(this, "Delete Failed",
                QString("Could not delete '%1'.").arg(editorId));
        }
    }
}

void ObjectWindowDialog::filterChanged(const QString& text)
{
    if (mModel)
    {
        mModel->applyFilter(text);
    }
}

void ObjectWindowDialog::cloneSelected()
{
    QModelIndex index = mTreeView->currentIndex();
    if (!index.isValid() || !mModel->parent(index).isValid())
        return;

    int categoryId = mModel->getCategoryIndex(index);
    int recordIndex = mModel->getRecordIndex(index);
    QString editorId = mModel->getRecordEditorId(categoryId, recordIndex);
    CkId::Type type = static_cast<CkId::Type>(mModel->getCategoryType(categoryId));

    bool ok = false;
    QString newId = QInputDialog::getText(this, "Clone Record",
        QString("New Editor ID for clone of '%1':").arg(editorId),
        QLineEdit::Normal, editorId + "_clone", &ok);

    if (ok && !newId.isEmpty())
    {
        if (mData->cloneRecordWithUndo(type, editorId, newId))
        {
            LOG_INFO(QString("Record '%1' cloned to '%2'").arg(editorId, newId));
            mModel->setData(mData);
        }
        else
        {
            QMessageBox::warning(this, "Clone Failed",
                QString("Could not clone '%1'.\n\nID '%2' may already exist.").arg(editorId, newId));
        }
    }
}

void ObjectWindowDialog::onDoubleClick(const QModelIndex& index)
{
    if (!index.isValid() || mModel->parent(index).isValid())
    {
        editSelected();
    }
}

QString ObjectWindowDialog::getModelPathForRecord(int categoryId, int recordIndex) const
{
    CkId::Type type = static_cast<CkId::Type>(mModel->getCategoryType(categoryId));

    switch (type)
    {
    case CkId::Type_Weap_:
    {
        auto& collection = mData->getWeaponCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
            return collection.getRecord(recordIndex).get().modelPath;
        break;
    }
    case CkId::Type_Armor_:
    {
        auto& collection = mData->getArmorCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
            return collection.getRecord(recordIndex).get().modelPath;
        break;
    }
    case CkId::Type_Stat_:
    {
        auto& collection = mData->getStatCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
            return collection.getRecord(recordIndex).get().modelPath;
        break;
    }
    case CkId::Type_Tree_:
    {
        auto& collection = mData->getTreeCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
            return collection.getRecord(recordIndex).get().modelPath;
        break;
    }
    case CkId::Type_Acti_:
    {
        auto& collection = mData->getActiCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
            return collection.getRecord(recordIndex).get().modelPath;
        break;
    }
    case CkId::Type_Misc_:
    {
        auto& collection = mData->getMiscCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
            return collection.getRecord(recordIndex).get().modelPath;
        break;
    }
    case CkId::Type_Book_:
    {
        auto& collection = mData->getBookCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
            return collection.getRecord(recordIndex).get().modelPath;
        break;
    }
    case CkId::Type_Ingr_:
    {
        auto& collection = mData->getIngrCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
            return collection.getRecord(recordIndex).get().modelPath;
        break;
    }
    case CkId::Type_Alch_:
    {
        auto& collection = mData->getAlchCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
            return collection.getRecord(recordIndex).get().modelPath;
        break;
    }
    case CkId::Type_Cont_:
    {
        auto& collection = mData->getContCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
            return collection.getRecord(recordIndex).get().modelPath;
        break;
    }
    default:
        break;
    }

    return QString();
}

void ObjectWindowDialog::openInBlender()
{
    QModelIndex index = mTreeView->currentIndex();
    if (!index.isValid() || !mModel->parent(index).isValid())
        return;

    int categoryId = mModel->getCategoryIndex(index);
    int recordIndex = mModel->getRecordIndex(index);
    QString editorId = mModel->getRecordEditorId(categoryId, recordIndex);
    CkId::Type type = static_cast<CkId::Type>(mModel->getCategoryType(categoryId));

    QString modelPath = getModelPathForRecord(categoryId, recordIndex);

    if (modelPath.isEmpty())
    {
        QMessageBox::warning(this, "No Model",
            QString("The record '%1' does not have a 3D model assigned.\n\n"
                    "Please set a model path in the editor first.")
            .arg(editorId));
        return;
    }

    QFileInfo fileInfo(modelPath);
    if (!fileInfo.exists())
    {
        QMessageBox::warning(this, "Model Not Found",
            QString("The model file could not be found:\n\n%1\n\n"
                    "Please verify the model path is correct.")
            .arg(modelPath));
        return;
    }

    if (BlenderLauncher::openInBlender(modelPath))
    {
        LOG_INFO(QString("Opened %1 (%2) in Blender").arg(editorId).arg(CkId(type).getTypeName()));
    }
    else
    {
        QMessageBox::critical(this, "Blender Error",
            "Failed to open file in Blender.\n\n"
            "Please ensure Blender is installed and the path is configured correctly.");
    }
}

void ObjectWindowDialog::previewNif()
{
    QModelIndex index = mTreeView->currentIndex();
    if (!index.isValid() || !mModel->parent(index).isValid())
        return;

    int categoryId = mModel->getCategoryIndex(index);
    int recordIndex = mModel->getRecordIndex(index);
    QString editorId = mModel->getRecordEditorId(categoryId, recordIndex);

    QString modelPath = getModelPathForRecord(categoryId, recordIndex);

    if (modelPath.isEmpty())
    {
        QMessageBox::warning(this, "No Model",
            QString("The record '%1' does not have a 3D model assigned.\n\n"
                    "Please set a model path in the editor first.")
            .arg(editorId));
        return;
    }

    QFileInfo fileInfo(modelPath);
    if (!fileInfo.exists())
    {
        QMessageBox::warning(this, "Model Not Found",
            QString("The model file could not be found:\n\n%1\n\n"
                    "Please verify the model path is correct.")
            .arg(modelPath));
        return;
    }

    NifPreviewDialog previewDialog(modelPath, this);
    previewDialog.exec();
}

void ObjectWindowDialog::compareNifs()
{
    QModelIndex index1 = mTreeView->currentIndex();
    if (!index1.isValid() || !mModel->parent(index1).isValid())
        return;

    int categoryId1 = mModel->getCategoryIndex(index1);
    int recordIndex1 = mModel->getRecordIndex(index1);
    QString editorId1 = mModel->getRecordEditorId(categoryId1, recordIndex1);

    QString modelPath1 = getModelPathForRecord(categoryId1, recordIndex1);

    if (modelPath1.isEmpty())
    {
        QMessageBox::warning(this, "No Model",
            QString("The record '%1' does not have a 3D model assigned.\n\n"
                    "Please set a model path in the editor first.")
            .arg(editorId1));
        return;
    }

    QFileInfo fileInfo1(modelPath1);
    if (!fileInfo1.exists())
    {
        QMessageBox::warning(this, "Model Not Found",
            QString("The model file could not be found:\n\n%1\n\n"
                    "Please verify the model path is correct.")
            .arg(modelPath1));
        return;
    }

    QString modelPath2 = QFileDialog::getOpenFileName(this, "Select Second NIF File", "", "NIF Files (*.nif)");
    if (modelPath2.isEmpty())
        return;

    QFileInfo fileInfo2(modelPath2);
    if (!fileInfo2.exists())
    {
        QMessageBox::warning(this, "Model Not Found",
            QString("The second model file could not be found:\n\n%1\n\n"
                    "Please verify the model path is correct.")
            .arg(modelPath2));
        return;
    }

    NifComparisonDialog compareDialog(modelPath1, modelPath2, this);
    compareDialog.exec();
}

QModelIndex ObjectWindowDialog::currentIndex() const
{
    return mTreeView->currentIndex();
}

int ObjectWindowDialog::getSelectedCategoryId(const QModelIndex& index) const
{
    if (!index.isValid() || !mModel) {
        return -1;
    }
    return mModel->getCategoryIndex(index);
}

ObjectWindowDialog::ClipboardRecord ObjectWindowDialog::sClipboardData;
bool ObjectWindowDialog::sHasClipboardData = false;

bool ObjectWindowDialog::hasClipboardData()
{
    return sHasClipboardData;
}

ObjectWindowDialog::ClipboardRecord ObjectWindowDialog::getClipboardData()
{
    return sClipboardData;
}

void ObjectWindowDialog::setClipboardData(const ClipboardRecord& record)
{
    sClipboardData = record;
    sHasClipboardData = true;
}

void ObjectWindowDialog::clearClipboardData()
{
    sHasClipboardData = false;
}

void ObjectWindowDialog::copyRecord()
{
    QModelIndex index = mTreeView->currentIndex();
    if (!index.isValid() || !mModel->parent(index).isValid())
        return;

    int categoryId = mModel->getCategoryIndex(index);
    int recordIndex = mModel->getRecordIndex(index);
    QString editorId = mModel->getRecordEditorId(categoryId, recordIndex);
    CkId::Type type = static_cast<CkId::Type>(mModel->getCategoryType(categoryId));

    ClipboardRecord record;
    record.recordType = static_cast<int>(type);
    record.editorId = editorId;

    switch (type)
    {
    case CkId::Type_Npc_:
    {
        auto& collection = mData->getNpcCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const NpcRecord& npc = collection.getRecord(recordIndex).get();
            record.formId = npc.formId;
            record.fields["fullName"] = npc.fullName;
            record.fields["level"] = static_cast<int>(npc.level);
            record.fields["health"] = static_cast<double>(npc.health);
            record.fields["magicka"] = static_cast<double>(npc.magicka);
            record.fields["stamina"] = static_cast<double>(npc.stamina);
            record.fields["intelligence"] = static_cast<int>(npc.intelligence);
            record.fields["race"] = static_cast<int>(npc.race);
            record.fields["sex"] = static_cast<int>(npc.sex);
            record.fields["class"] = static_cast<int>(npc.class_);
            record.fields["faction"] = static_cast<int>(npc.faction);
            setClipboardData(record);
            LOG_INFO(QString("NPC '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Weap_:
    {
        auto& collection = mData->getWeaponCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const WeaponRecord& weapon = collection.getRecord(recordIndex).get();
            record.formId = weapon.formId;
            record.fields["damage"] = static_cast<double>(weapon.damage);
            record.fields["speed"] = static_cast<double>(weapon.speed);
            record.fields["reach"] = static_cast<double>(weapon.reach);
            record.fields["weight"] = static_cast<double>(weapon.weight);
            record.fields["value"] = static_cast<int>(weapon.value);
            record.fields["weaponType"] = static_cast<int>(weapon.weaponType);
            setClipboardData(record);
            LOG_INFO(QString("Weapon '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Armor_:
    {
        auto& collection = mData->getArmorCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const ArmorRecord& armor = collection.getRecord(recordIndex).get();
            record.formId = armor.formId;
            record.fields["armorRating"] = static_cast<int>(armor.armorRating);
            record.fields["weight"] = static_cast<double>(armor.weight);
            record.fields["value"] = static_cast<int>(armor.value);
            record.fields["health"] = static_cast<double>(armor.health);
            setClipboardData(record);
            LOG_INFO(QString("Armor '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Spel_:
    {
        auto& collection = mData->getSpellCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const SpellRecord& spell = collection.getRecord(recordIndex).get();
            record.formId = spell.formId;
            record.fields["cost"] = static_cast<int>(spell.cost);
            record.fields["castingSound"] = static_cast<int>(spell.castingSound);
            setClipboardData(record);
            LOG_INFO(QString("Spell '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Quest_:
    {
        auto& collection = mData->getQuestCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const QuestRecord& quest = collection.getRecord(recordIndex).get();
            record.formId = quest.formId;
            record.fields["questName"] = quest.questName;
            record.fields["questDesc"] = quest.questDesc;
            record.fields["questType"] = static_cast<int>(quest.questType);
            record.fields["dialogueView"] = quest.dialogueView;
            setClipboardData(record);
            LOG_INFO(QString("Quest '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Glob_:
    {
        auto& collection = mData->getGlobCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const GlobalVariable& glob = collection.getRecord(recordIndex).get();
            record.fields["value"] = glob.value.getFloat();
            setClipboardData(record);
            LOG_INFO(QString("Global '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Tree_:
    {
        auto& collection = mData->getTreeCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const TreeRecord& tree = collection.getRecord(recordIndex).get();
            record.formId = tree.formId;
            record.fields["modelPath"] = tree.modelPath;
            setClipboardData(record);
            LOG_INFO(QString("Tree '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Stat_:
    {
        auto& collection = mData->getStatCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const StatRecord& stat = collection.getRecord(recordIndex).get();
            record.formId = stat.formId;
            record.fields["modelPath"] = stat.modelPath;
            setClipboardData(record);
            LOG_INFO(QString("Stat '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Acti_:
    {
        auto& collection = mData->getActiCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const ActiRecord& acti = collection.getRecord(recordIndex).get();
            record.formId = acti.formId;
            record.fields["iconPath"] = acti.iconPath;
            record.fields["modelPath"] = acti.modelPath;
            setClipboardData(record);
            LOG_INFO(QString("Acti '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Misc_:
    {
        auto& collection = mData->getMiscCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const MiscRecord& misc = collection.getRecord(recordIndex).get();
            record.formId = misc.formId;
            record.fields["iconPath"] = misc.iconPath;
            record.fields["modelPath"] = misc.modelPath;
            record.fields["weight"] = static_cast<double>(misc.weight);
            record.fields["value"] = static_cast<int>(misc.value);
            setClipboardData(record);
            LOG_INFO(QString("Misc '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Alch_:
    {
        auto& collection = mData->getAlchCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const AlchRecord& alch = collection.getRecord(recordIndex).get();
            record.formId = alch.formId;
            record.fields["iconPath"] = alch.iconPath;
            record.fields["modelPath"] = alch.modelPath;
            record.fields["weight"] = static_cast<double>(alch.weight);
            record.fields["value"] = static_cast<int>(alch.value);
            setClipboardData(record);
            LOG_INFO(QString("Alch '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Ingr_:
    {
        auto& collection = mData->getIngrCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const IngrRecord& ingr = collection.getRecord(recordIndex).get();
            record.formId = ingr.formId;
            record.fields["iconPath"] = ingr.iconPath;
            record.fields["modelPath"] = ingr.modelPath;
            record.fields["weight"] = static_cast<double>(ingr.weight);
            record.fields["value"] = static_cast<int>(ingr.value);
            setClipboardData(record);
            LOG_INFO(QString("Ingr '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Book_:
    {
        auto& collection = mData->getBookCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const BookRecord& book = collection.getRecord(recordIndex).get();
            record.formId = book.formId;
            record.fields["iconPath"] = book.iconPath;
            record.fields["modelPath"] = book.modelPath;
            record.fields["pageCount"] = static_cast<int>(book.pageCount);
            record.fields["pages"] = book.pages;
            setClipboardData(record);
            LOG_INFO(QString("Book '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Ench_:
    {
        auto& collection = mData->getEnchCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const EnchRecord& ench = collection.getRecord(recordIndex).get();
            record.formId = ench.formId;
            record.fields["name"] = ench.name;
            record.fields["costLimit"] = static_cast<int>(ench.costLimit);
            record.fields["charges"] = static_cast<int>(ench.charges);
            record.fields["enchantmentData"] = static_cast<int>(ench.enchantmentData);
            setClipboardData(record);
            LOG_INFO(QString("Ench '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Cont_:
    {
        auto& collection = mData->getContCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const ContRecord& cont = collection.getRecord(recordIndex).get();
            record.formId = cont.formId;
            record.fields["iconPath"] = cont.iconPath;
            record.fields["modelPath"] = cont.modelPath;
            record.fields["contents"] = static_cast<int>(cont.contents);
            record.fields["inventoryControl"] = static_cast<int>(cont.inventoryControl);
            record.fields["weight"] = static_cast<double>(cont.weight);
            record.fields["value"] = static_cast<int>(cont.value);
            setClipboardData(record);
            LOG_INFO(QString("Cont '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Race_:
    {
        auto& collection = mData->getRaceCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const RaceRecord& race = collection.getRecord(recordIndex).get();
            record.formId = race.formId;
            record.fields["raceFlags"] = static_cast<int>(race.raceFlags);
            setClipboardData(record);
            LOG_INFO(QString("Race '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_PerK_:
    {
        auto& collection = mData->getPerkCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const PerkRecord& perk = collection.getRecord(recordIndex).get();
            record.formId = perk.formId;
            record.fields["description"] = perk.description;
            record.fields["requirements"] = perk.requirements;
            record.fields["iconPath"] = perk.iconPath;
            setClipboardData(record);
            LOG_INFO(QString("Perk '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Magic_:
    {
        auto& collection = mData->getMagicCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const MagicRecord& magic = collection.getRecord(recordIndex).get();
            record.formId = magic.formId;
            record.fields["schools"] = static_cast<int>(magic.schools);
            record.fields["damageType"] = static_cast<int>(magic.damageType);
            record.fields["castingSound"] = static_cast<int>(magic.castingSound);
            record.fields["iconPath"] = magic.iconPath;
            record.fields["modelPath"] = magic.modelPath;
            setClipboardData(record);
            LOG_INFO(QString("Magic '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Pack_:
    {
        auto& collection = mData->getPackCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const PackageRecord& pack = collection.getRecord(recordIndex).get();
            record.formId = pack.formId;
            record.fields["packageType"] = static_cast<int>(pack.packageType);
            record.fields["targetType"] = static_cast<int>(pack.targetType);
            setClipboardData(record);
            LOG_INFO(QString("Package '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Lcrt_:
    {
        auto& collection = mData->getLcrtCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const LocationRefType& lcrt = collection.getRecord(recordIndex).get();
            record.formId = 0;
            record.fields["color"] = static_cast<int>(lcrt.color);
            setClipboardData(record);
            LOG_INFO(QString("LocationRef '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Class_:
    {
        auto& collection = mData->getClassCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const ClassRecord& classRec = collection.getRecord(recordIndex).get();
            record.formId = classRec.formId;
            record.fields["className"] = classRec.className;
            record.fields["description"] = classRec.description;
            record.fields["serviceFlags"] = static_cast<int>(classRec.serviceFlags);
            record.fields["iconPath"] = classRec.iconPath;
            setClipboardData(record);
            LOG_INFO(QString("Class '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Cel_:
    {
        auto& collection = mData->getCellCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const CellRecord& cellRec = collection.getRecord(recordIndex).get();
            record.formId = cellRec.formId;
            record.fields["editorId"] = cellRec.editorId;
            record.fields["cellName"] = cellRec.cellName;
            record.fields["cellX"] = static_cast<int>(cellRec.cellX);
            record.fields["cellY"] = static_cast<int>(cellRec.cellY);
            record.fields["owner"] = static_cast<int>(cellRec.owner);
            record.fields["lockLevel"] = static_cast<int>(cellRec.lockLevel);
            setClipboardData(record);
            LOG_INFO(QString("Cell '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_WRLD_:
    {
        auto& collection = mData->getWorldspaceCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const WorldspaceRecord& wrldRec = collection.getRecord(recordIndex).get();
            record.formId = wrldRec.formId;
            record.fields["editorId"] = wrldRec.editorId;
            record.fields["waterType"] = static_cast<int>(wrldRec.waterType);
            setClipboardData(record);
            LOG_INFO(QString("Worldspace '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_LOCT_:
    {
        auto& collection = mData->getLocationCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const LocationRecord& locRec = collection.getRecord(recordIndex).get();
            record.formId = locRec.formId;
            record.fields["editorId"] = locRec.editorId;
            record.fields["locationName"] = locRec.locationName;
            record.fields["parentId"] = static_cast<int>(locRec.parentId);
            record.fields["x"] = static_cast<int>(locRec.x);
            record.fields["y"] = static_cast<int>(locRec.y);
            record.fields["z"] = static_cast<int>(locRec.z);
            setClipboardData(record);
            LOG_INFO(QString("Location '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Refr_:
    {
        auto& collection = mData->getRefrCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const RefrRecord& refRec = collection.getRecord(recordIndex).get();
            record.formId = refRec.formId;
            record.fields["baseId"] = static_cast<int>(refRec.baseId);
            record.fields["posX"] = refRec.posX;
            record.fields["posY"] = refRec.posY;
            record.fields["posZ"] = refRec.posZ;
            record.fields["rotX"] = refRec.rotX;
            record.fields["rotY"] = refRec.rotY;
            record.fields["rotZ"] = refRec.rotZ;
            record.fields["scale"] = refRec.scale;
            record.fields["owner"] = static_cast<int>(refRec.owner);
            record.fields["lockLevel"] = static_cast<int>(refRec.lockLevel);
            record.fields["initiallyDisabled"] = refRec.initiallyDisabled;
            setClipboardData(record);
            LOG_INFO(QString("Reference '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Dial_:
    {
        auto& collection = mData->getDialCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const DialRecord& dial = collection.getRecord(recordIndex).get();
            record.formId = dial.formId;
            record.fields["editorId"] = dial.editorId;
            record.fields["topicName"] = dial.topicName;
            setClipboardData(record);
            LOG_INFO(QString("Dialogue '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    case CkId::Type_Info_:
    {
        auto& collection = mData->getInfoCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            const InfoRecord& info = collection.getRecord(recordIndex).get();
            record.formId = info.formId;
            record.fields["editorId"] = info.editorId;
            record.fields["responseText"] = info.responseText;
            setClipboardData(record);
            LOG_INFO(QString("Dialogue Info '%1' copied to clipboard").arg(editorId));
        }
        break;
    }
    default:
    {
        QMessageBox::information(this, "Copy",
            QString("Copying %1 records is not yet supported.\nSupported: NPC_, WEAP_, ARMOR_, SPEL_, QUEST_, GLOB_, TREE_, STAT_, ACTI_, MISC_, ALCH_, INGR_, BOOK_, ENCH_, CONT_, RACE_, PERK_, MAGIC_, PACK_, LCRT_, CLASS_, CELL_, WRLD_, LOCT_, REFR_, DIAL_, INFO_")
            .arg(CkId(type).getTypeName()));
        return;
    }
    }

    if (sHasClipboardData)
    {
        QMessageBox::information(this, "Copied",
            QString("Record '%1' copied to clipboard.").arg(editorId));
    }
}

void ObjectWindowDialog::cutRecord()
{
    copyRecord();
    if (sHasClipboardData)
    {
        QModelIndex index = mTreeView->currentIndex();
        if (!index.isValid() || !mModel->parent(index).isValid())
            return;

        int categoryId = mModel->getCategoryIndex(index);
        int recordIndex = mModel->getRecordIndex(index);
        QString editorId = mModel->getRecordEditorId(categoryId, recordIndex);
        CkId::Type type = static_cast<CkId::Type>(mModel->getCategoryType(categoryId));

        auto result = QMessageBox::question(this, "Cut Record",
            QString("Cut '%1' from the object window?\n\n"
                    "The record will be removed from this location and stored in the clipboard.\n"
                    "Use Paste to insert it into another plugin.")
                .arg(editorId),
            QMessageBox::Yes | QMessageBox::No);

        if (result == QMessageBox::Yes)
        {
            if (mData->removeRecord(type, editorId))
            {
                LOG_INFO(QString("Record '%1' cut to clipboard").arg(editorId));
                mModel->setData(mData);
            }
            else
            {
                QMessageBox::warning(this, "Cut Failed",
                    QString("Could not cut '%1'.").arg(editorId));
            }
        }
        else
        {
            clearClipboardData();
        }
    }
}

void ObjectWindowDialog::pasteRecord()
{
    if (!sHasClipboardData)
    {
        QMessageBox::information(this, "Paste", "Nothing in the clipboard.");
        return;
    }

    ClipboardRecord clipData = getClipboardData();
    CkId::Type type = static_cast<CkId::Type>(clipData.recordType);

    bool ok = false;
    QString newId = QInputDialog::getText(this, "Paste Record",
        QString("Editor ID for pasted record (type: %1):").arg(CkId(type).getTypeName()),
        QLineEdit::Normal, clipData.editorId + "_paste", &ok);

    if (!ok || newId.isEmpty())
        return;

    bool created = false;

    switch (type)
    {
    case CkId::Type_Npc_:
    {
        auto& collection = mData->getNpcCollection();
        NpcRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("fullName"))
            newRecord.fullName = clipData.fields["fullName"].toString();
        if (clipData.fields.contains("level"))
            newRecord.level = static_cast<quint32>(clipData.fields["level"].toInt());
        if (clipData.fields.contains("health"))
            newRecord.health = static_cast<quint32>(clipData.fields["health"].toDouble());
        if (clipData.fields.contains("magicka"))
            newRecord.magicka = static_cast<quint32>(clipData.fields["magicka"].toDouble());
        if (clipData.fields.contains("stamina"))
            newRecord.stamina = static_cast<quint32>(clipData.fields["stamina"].toDouble());
        if (clipData.fields.contains("intelligence"))
            newRecord.intelligence = static_cast<quint32>(clipData.fields["intelligence"].toInt());
        if (clipData.fields.contains("race"))
            newRecord.race = static_cast<quint32>(clipData.fields["race"].toInt());
        if (clipData.fields.contains("sex"))
            newRecord.sex = static_cast<quint32>(clipData.fields["sex"].toInt());
        if (clipData.fields.contains("class"))
            newRecord.class_ = static_cast<quint32>(clipData.fields["class"].toInt());
        if (clipData.fields.contains("faction"))
            newRecord.faction = static_cast<quint32>(clipData.fields["faction"].toInt());

        if (mData->addNpc(newRecord))
        {
            LOG_INFO(QString("NPC '%1' pasted").arg(newId));
            created = true;
        }
        break;
    }
    case CkId::Type_Weap_:
    {
        auto& collection = mData->getWeaponCollection();
        WeaponRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("damage"))
            newRecord.damage = static_cast<float>(clipData.fields["damage"].toDouble());
        if (clipData.fields.contains("speed"))
            newRecord.speed = static_cast<float>(clipData.fields["speed"].toDouble());
        if (clipData.fields.contains("reach"))
            newRecord.reach = static_cast<float>(clipData.fields["reach"].toDouble());
        if (clipData.fields.contains("weight"))
            newRecord.weight = static_cast<float>(clipData.fields["weight"].toDouble());
        if (clipData.fields.contains("value"))
            newRecord.value = static_cast<quint32>(clipData.fields["value"].toInt());
        if (clipData.fields.contains("weaponType"))
            newRecord.weaponType = static_cast<quint32>(clipData.fields["weaponType"].toInt());

        if (mData->addWeapon(newRecord))
        {
            LOG_INFO(QString("Weapon '%1' pasted").arg(newId));
            created = true;
        }
        break;
    }
    case CkId::Type_Armor_:
    {
        auto& collection = mData->getArmorCollection();
        ArmorRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("armorRating"))
            newRecord.armorRating = static_cast<quint32>(clipData.fields["armorRating"].toInt());
        if (clipData.fields.contains("weight"))
            newRecord.weight = static_cast<float>(clipData.fields["weight"].toDouble());
        if (clipData.fields.contains("value"))
            newRecord.value = static_cast<quint32>(clipData.fields["value"].toInt());
        if (clipData.fields.contains("health"))
            newRecord.health = static_cast<float>(clipData.fields["health"].toDouble());

        if (mData->addArmor(newRecord))
        {
            LOG_INFO(QString("Armor '%1' pasted").arg(newId));
            created = true;
        }
        break;
    }
    case CkId::Type_Spel_:
    {
        auto& collection = mData->getSpellCollection();
        SpellRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("cost"))
            newRecord.cost = static_cast<quint32>(clipData.fields["cost"].toInt());
        if (clipData.fields.contains("castingSound"))
            newRecord.castingSound = static_cast<quint32>(clipData.fields["castingSound"].toInt());

        if (mData->addSpell(newRecord))
        {
            LOG_INFO(QString("Spell '%1' pasted").arg(newId));
            created = true;
        }
        break;
    }
    case CkId::Type_Quest_:
    {
        auto& collection = mData->getQuestCollection();
        QuestRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("questName"))
            newRecord.questName = clipData.fields["questName"].toString();
        if (clipData.fields.contains("questDesc"))
            newRecord.questDesc = clipData.fields["questDesc"].toString();
        if (clipData.fields.contains("questType"))
            newRecord.questType = static_cast<quint32>(clipData.fields["questType"].toInt());
        if (clipData.fields.contains("dialogueView"))
            newRecord.dialogueView = clipData.fields["dialogueView"].toString();

        if (mData->addQuest(newRecord))
        {
            LOG_INFO(QString("Quest '%1' pasted").arg(newId));
            created = true;
        }
        break;
    }
    case CkId::Type_Glob_:
    {
        auto& collection = mData->getGlobCollection();
        GlobalVariable newRecord;
        newRecord.editorId = newId;
        if (clipData.fields.contains("value"))
            newRecord.value.setFloat(static_cast<float>(clipData.fields["value"].toDouble()));
        collection.add(newRecord);
        LOG_INFO(QString("Global '%1' pasted").arg(newId));
        created = true;
        break;
    }
    case CkId::Type_Tree_:
    {
        auto& collection = mData->getTreeCollection();
        TreeRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("modelPath"))
            newRecord.modelPath = clipData.fields["modelPath"].toString();
        newRecord.rawSubRecords.clear();
        collection.add(newRecord);
        LOG_INFO(QString("Tree '%1' pasted").arg(newId));
        created = true;
        break;
    }
    case CkId::Type_Stat_:
    {
        auto& collection = mData->getStatCollection();
        StatRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("modelPath"))
            newRecord.modelPath = clipData.fields["modelPath"].toString();
        newRecord.rawSubRecords.clear();
        collection.add(newRecord);
        LOG_INFO(QString("Stat '%1' pasted").arg(newId));
        created = true;
        break;
    }
    case CkId::Type_Acti_:
    {
        auto& collection = mData->getActiCollection();
        ActiRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("iconPath"))
            newRecord.iconPath = clipData.fields["iconPath"].toString();
        if (clipData.fields.contains("modelPath"))
            newRecord.modelPath = clipData.fields["modelPath"].toString();
        newRecord.rawSubRecords.clear();
        collection.add(newRecord);
        LOG_INFO(QString("Acti '%1' pasted").arg(newId));
        created = true;
        break;
    }
    case CkId::Type_Misc_:
    {
        auto& collection = mData->getMiscCollection();
        MiscRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("iconPath"))
            newRecord.iconPath = clipData.fields["iconPath"].toString();
        if (clipData.fields.contains("modelPath"))
            newRecord.modelPath = clipData.fields["modelPath"].toString();
        if (clipData.fields.contains("weight"))
            newRecord.weight = static_cast<float>(clipData.fields["weight"].toDouble());
        if (clipData.fields.contains("value"))
            newRecord.value = static_cast<quint32>(clipData.fields["value"].toInt());
        newRecord.rawSubRecords.clear();
        collection.add(newRecord);
        LOG_INFO(QString("Misc '%1' pasted").arg(newId));
        created = true;
        break;
    }
    case CkId::Type_Alch_:
    {
        auto& collection = mData->getAlchCollection();
        AlchRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("iconPath"))
            newRecord.iconPath = clipData.fields["iconPath"].toString();
        if (clipData.fields.contains("modelPath"))
            newRecord.modelPath = clipData.fields["modelPath"].toString();
        if (clipData.fields.contains("weight"))
            newRecord.weight = static_cast<float>(clipData.fields["weight"].toDouble());
        if (clipData.fields.contains("value"))
            newRecord.value = static_cast<quint32>(clipData.fields["value"].toInt());
        newRecord.rawSubRecords.clear();
        collection.add(newRecord);
        LOG_INFO(QString("Alch '%1' pasted").arg(newId));
        created = true;
        break;
    }
    case CkId::Type_Ingr_:
    {
        auto& collection = mData->getIngrCollection();
        IngrRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("iconPath"))
            newRecord.iconPath = clipData.fields["iconPath"].toString();
        if (clipData.fields.contains("modelPath"))
            newRecord.modelPath = clipData.fields["modelPath"].toString();
        if (clipData.fields.contains("weight"))
            newRecord.weight = static_cast<float>(clipData.fields["weight"].toDouble());
        if (clipData.fields.contains("value"))
            newRecord.value = static_cast<quint32>(clipData.fields["value"].toInt());
        newRecord.rawSubRecords.clear();
        collection.add(newRecord);
        LOG_INFO(QString("Ingr '%1' pasted").arg(newId));
        created = true;
        break;
    }
    case CkId::Type_Book_:
    {
        auto& collection = mData->getBookCollection();
        BookRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("iconPath"))
            newRecord.iconPath = clipData.fields["iconPath"].toString();
        if (clipData.fields.contains("modelPath"))
            newRecord.modelPath = clipData.fields["modelPath"].toString();
        if (clipData.fields.contains("pageCount"))
            newRecord.pageCount = clipData.fields["pageCount"].toInt();
        if (clipData.fields.contains("pages"))
            newRecord.pages = clipData.fields["pages"].toString();
        newRecord.rawSubRecords.clear();
        collection.add(newRecord);
        LOG_INFO(QString("Book '%1' pasted").arg(newId));
        created = true;
        break;
    }
    case CkId::Type_Ench_:
    {
        auto& collection = mData->getEnchCollection();
        EnchRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("name"))
            newRecord.name = clipData.fields["name"].toString();
        if (clipData.fields.contains("costLimit"))
            newRecord.costLimit = static_cast<quint32>(clipData.fields["costLimit"].toInt());
        if (clipData.fields.contains("charges"))
            newRecord.charges = static_cast<quint32>(clipData.fields["charges"].toInt());
        if (clipData.fields.contains("enchantmentData"))
            newRecord.enchantmentData = static_cast<quint32>(clipData.fields["enchantmentData"].toInt());
        newRecord.rawSubRecords.clear();
        collection.add(newRecord);
        LOG_INFO(QString("Ench '%1' pasted").arg(newId));
        created = true;
        break;
    }
    case CkId::Type_Cont_:
    {
        auto& collection = mData->getContCollection();
        ContRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("iconPath"))
            newRecord.iconPath = clipData.fields["iconPath"].toString();
        if (clipData.fields.contains("modelPath"))
            newRecord.modelPath = clipData.fields["modelPath"].toString();
        if (clipData.fields.contains("contents"))
            newRecord.contents = static_cast<quint32>(clipData.fields["contents"].toInt());
        if (clipData.fields.contains("inventoryControl"))
            newRecord.inventoryControl = static_cast<quint32>(clipData.fields["inventoryControl"].toInt());
        if (clipData.fields.contains("weight"))
            newRecord.weight = static_cast<float>(clipData.fields["weight"].toDouble());
        if (clipData.fields.contains("value"))
            newRecord.value = static_cast<quint32>(clipData.fields["value"].toInt());
        newRecord.rawSubRecords.clear();
        collection.add(newRecord);
        LOG_INFO(QString("Cont '%1' pasted").arg(newId));
        created = true;
        break;
    }
    case CkId::Type_Race_:
    {
        auto& collection = mData->getRaceCollection();
        RaceRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("raceFlags"))
            newRecord.raceFlags = static_cast<quint32>(clipData.fields["raceFlags"].toInt());
        newRecord.rawSubRecords.clear();
        collection.add(newRecord);
        LOG_INFO(QString("Race '%1' pasted").arg(newId));
        created = true;
        break;
    }
    case CkId::Type_PerK_:
    {
        auto& collection = mData->getPerkCollection();
        PerkRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("description"))
            newRecord.description = clipData.fields["description"].toString();
        if (clipData.fields.contains("requirements"))
            newRecord.requirements = clipData.fields["requirements"].toString();
        if (clipData.fields.contains("iconPath"))
            newRecord.iconPath = clipData.fields["iconPath"].toString();
        newRecord.rawSubRecords.clear();
        collection.add(newRecord);
        LOG_INFO(QString("Perk '%1' pasted").arg(newId));
        created = true;
        break;
    }
    case CkId::Type_Magic_:
    {
        auto& collection = mData->getMagicCollection();
        MagicRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("schools"))
            newRecord.schools = static_cast<quint32>(clipData.fields["schools"].toInt());
        if (clipData.fields.contains("damageType"))
            newRecord.damageType = static_cast<quint32>(clipData.fields["damageType"].toInt());
        if (clipData.fields.contains("castingSound"))
            newRecord.castingSound = static_cast<quint32>(clipData.fields["castingSound"].toInt());
        if (clipData.fields.contains("iconPath"))
            newRecord.iconPath = clipData.fields["iconPath"].toString();
        if (clipData.fields.contains("modelPath"))
            newRecord.modelPath = clipData.fields["modelPath"].toString();
        newRecord.rawSubRecords.clear();
        collection.add(newRecord);
        LOG_INFO(QString("Magic '%1' pasted").arg(newId));
        created = true;
        break;
    }
    case CkId::Type_Pack_:
    {
        auto& collection = mData->getPackCollection();
        PackageRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("packageType"))
            newRecord.packageType = static_cast<quint32>(clipData.fields["packageType"].toInt());
        if (clipData.fields.contains("targetType"))
            newRecord.targetType = static_cast<quint32>(clipData.fields["targetType"].toInt());
        newRecord.rawSubRecords.clear();
        collection.add(newRecord);
        LOG_INFO(QString("Package '%1' pasted").arg(newId));
        created = true;
        break;
    }
    case CkId::Type_Lcrt_:
    {
        auto& collection = mData->getLcrtCollection();
        LocationRefType newRecord;
        newRecord.editorId = newId;
        if (clipData.fields.contains("color"))
            newRecord.color = static_cast<uint32_t>(clipData.fields["color"].toInt());
        collection.add(newRecord);
        LOG_INFO(QString("LocationRef '%1' pasted").arg(newId));
        created = true;
        break;
    }
    case CkId::Type_Class_:
    {
        auto& collection = mData->getClassCollection();
        ClassRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("className"))
            newRecord.className = clipData.fields["className"].toString();
        if (clipData.fields.contains("description"))
            newRecord.description = clipData.fields["description"].toString();
        if (clipData.fields.contains("serviceFlags"))
            newRecord.serviceFlags = static_cast<quint32>(clipData.fields["serviceFlags"].toInt());
        if (clipData.fields.contains("iconPath"))
            newRecord.iconPath = clipData.fields["iconPath"].toString();
        newRecord.rawSubRecords.clear();
        collection.add(newRecord);
        LOG_INFO(QString("Class '%1' pasted").arg(newId));
        created = true;
        break;
    }
    case CkId::Type_Cel_:
    {
        auto& collection = mData->getCellCollection();
        CellRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("cellName"))
            newRecord.cellName = clipData.fields["cellName"].toString();
        if (clipData.fields.contains("cellX"))
            newRecord.cellX = static_cast<qint32>(clipData.fields["cellX"].toInt());
        if (clipData.fields.contains("cellY"))
            newRecord.cellY = static_cast<qint32>(clipData.fields["cellY"].toInt());
        if (clipData.fields.contains("owner"))
            newRecord.owner = static_cast<quint32>(clipData.fields["owner"].toInt());
        if (clipData.fields.contains("lockLevel"))
            newRecord.lockLevel = static_cast<quint32>(clipData.fields["lockLevel"].toInt());
        newRecord.rawSubRecords.clear();
        if (mData->addCell(newRecord))
        {
            LOG_INFO(QString("Cell '%1' pasted").arg(newId));
            created = true;
        }
        break;
    }
    case CkId::Type_WRLD_:
    {
        auto& collection = mData->getWorldspaceCollection();
        WorldspaceRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("waterType"))
            newRecord.waterType = static_cast<quint32>(clipData.fields["waterType"].toInt());
        newRecord.rawSubRecords.clear();
        if (mData->addWorldspace(newRecord))
        {
            LOG_INFO(QString("Worldspace '%1' pasted").arg(newId));
            created = true;
        }
        break;
    }
    case CkId::Type_LOCT_:
    {
        auto& collection = mData->getLocationCollection();
        LocationRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("locationName"))
            newRecord.locationName = clipData.fields["locationName"].toString();
        if (clipData.fields.contains("parentId"))
            newRecord.parentId = static_cast<quint32>(clipData.fields["parentId"].toInt());
        if (clipData.fields.contains("x"))
            newRecord.x = static_cast<quint32>(clipData.fields["x"].toInt());
        if (clipData.fields.contains("y"))
            newRecord.y = static_cast<quint32>(clipData.fields["y"].toInt());
        if (clipData.fields.contains("z"))
            newRecord.z = static_cast<quint32>(clipData.fields["z"].toInt());
        newRecord.rawSubRecords.clear();
        if (mData->addLocation(newRecord))
        {
            LOG_INFO(QString("Location '%1' pasted").arg(newId));
            created = true;
        }
        break;
    }
    case CkId::Type_Refr_:
    {
        auto& collection = mData->getRefrCollection();
        RefrRecord newRecord;
        newRecord.formId = 0;
        if (clipData.fields.contains("baseId"))
            newRecord.baseId = static_cast<quint32>(clipData.fields["baseId"].toInt());
        if (clipData.fields.contains("posX"))
            newRecord.posX = static_cast<float>(clipData.fields["posX"].toDouble());
        if (clipData.fields.contains("posY"))
            newRecord.posY = static_cast<float>(clipData.fields["posY"].toDouble());
        if (clipData.fields.contains("posZ"))
            newRecord.posZ = static_cast<float>(clipData.fields["posZ"].toDouble());
        if (clipData.fields.contains("rotX"))
            newRecord.rotX = static_cast<float>(clipData.fields["rotX"].toDouble());
        if (clipData.fields.contains("rotY"))
            newRecord.rotY = static_cast<float>(clipData.fields["rotY"].toDouble());
        if (clipData.fields.contains("rotZ"))
            newRecord.rotZ = static_cast<float>(clipData.fields["rotZ"].toDouble());
        if (clipData.fields.contains("scale"))
            newRecord.scale = static_cast<float>(clipData.fields["scale"].toDouble());
        if (clipData.fields.contains("owner"))
            newRecord.owner = static_cast<quint32>(clipData.fields["owner"].toInt());
        if (clipData.fields.contains("lockLevel"))
            newRecord.lockLevel = static_cast<quint32>(clipData.fields["lockLevel"].toInt());
        if (clipData.fields.contains("initiallyDisabled"))
            newRecord.initiallyDisabled = clipData.fields["initiallyDisabled"].toBool();
        newRecord.rawSubRecords.clear();
        if (mData->addRef(newRecord))
        {
            LOG_INFO(QString("Reference '%1' pasted").arg(newId));
            created = true;
        }
        break;
    }
    case CkId::Type_Dial_:
    {
        auto& collection = mData->getDialCollection();
        DialRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("topicName"))
            newRecord.topicName = clipData.fields["topicName"].toString();
        
        if (mData->addDial(newRecord))
        {
            LOG_INFO(QString("Dialogue '%1' pasted").arg(newId));
            created = true;
        }
        break;
    }
    case CkId::Type_Info_:
    {
        auto& collection = mData->getInfoCollection();
        InfoRecord newRecord;
        newRecord.editorId = newId;
        newRecord.formId = 0;
        if (clipData.fields.contains("responseText"))
            newRecord.responseText = clipData.fields["responseText"].toString();
        
        if (mData->addInfo(newRecord))
        {
            LOG_INFO(QString("Dialogue Info '%1' pasted").arg(newId));
            created = true;
        }
        break;
    }
    default:
    {
        QMessageBox::information(this, "Paste",
            QString("Pasting %1 records is not yet supported.\nSupported: NPC_, WEAP_, ARMOR_, SPEL_, QUEST_, GLOB_, TREE_, STAT_, ACTI_, MISC_, ALCH_, INGR_, BOOK_, ENCH_, CONT_, RACE_, PERK_, MAGIC_, PACK_, LCRT_, CLASS_, CELL_, WRLD_, LOCT_, REFR_, DIAL_, INFO_")
            .arg(CkId(type).getTypeName()));
        return;
    }
    }

    if (created)
    {

        QMessageBox::information(this, "Paste",
            QString("Record '%1' pasted successfully.").arg(newId));
        mModel->setData(mData);
    }
    else
    {
        QMessageBox::warning(this, "Paste Failed",
            QString("Could not paste record as '%1'.\n\nThe ID may already exist.").arg(newId));
    }
}

void ObjectWindowDialog::enableMultiSelect(bool enabled)
{
    if (enabled)
        mTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    else
        mTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
}

QList<QModelIndex> ObjectWindowDialog::getSelectedIndices() const
{
    QList<QModelIndex> indices;
    QModelIndexList selected = mTreeView->selectionModel()->selectedIndexes();
    for (const QModelIndex& idx : selected)
    {
        if (idx.isValid() && mModel->parent(idx).isValid())
        {
            indices.append(idx);
        }
    }
    return indices;
}

void ObjectWindowDialog::batchSetEditorId()
{
    QList<QModelIndex> selectedIndices = getSelectedIndices();
    if (selectedIndices.isEmpty())
    {
        QMessageBox::warning(this, "No Selection",
            "Please select one or more records to batch edit.\n\nUse Ctrl+Click for multi-select.");
        return;
    }

    bool ok = false;
    QString newEditorId = QInputDialog::getText(this,
        "Batch Set EditorID",
        QString("Enter the same EditorID for %1 record(s):").arg(selectedIndices.count()),
        QLineEdit::Normal, "", &ok);

    if (!ok || newEditorId.isEmpty())
        return;

    int successCount = 0;

    for (const QModelIndex& idx : selectedIndices)
    {
        int categoryId = mModel->getCategoryIndex(idx);
        int recordIndex = mModel->getRecordIndex(idx);
        QString editorId = mModel->getRecordEditorId(categoryId, recordIndex);
        CkId::Type type = static_cast<CkId::Type>(mModel->getCategoryType(categoryId));

        if (mData->cloneRecord(type, editorId, newEditorId))
            successCount++;
    }

    mModel->setData(mData);
    mStatusLabel->setText(QString("Batch edit: %1 record(s) updated").arg(successCount));
}

void ObjectWindowDialog::batchDuplicateIds()
{
    QList<QModelIndex> selectedIndices = getSelectedIndices();
    if (selectedIndices.isEmpty())
    {
        QMessageBox::warning(this, "No Selection",
            "Please select one or more records to duplicate.\n\nUse Ctrl+Click for multi-select.");
        return;
    }

    bool ok = false;
    int offset = QInputDialog::getInt(this, "Batch Duplicate",
        "Enter ID offset (e.g. 1000):", 1000, 1, 99999, 1, &ok);

    if (!ok)
        return;

    int successCount = 0;
    int counter = 0;

    for (const QModelIndex& idx : selectedIndices)
    {
        int categoryId = mModel->getCategoryIndex(idx);
        int recordIndex = mModel->getRecordIndex(idx);
        QString editorId = mModel->getRecordEditorId(categoryId, recordIndex);
        CkId::Type type = static_cast<CkId::Type>(mModel->getCategoryType(categoryId));

        QString newId = editorId + "_clone" + QString::number(offset + counter++);

        if (mData->cloneRecord(type, editorId, newId))
            successCount++;
    }

    mModel->setData(mData);
    mStatusLabel->setText(QString("Batch duplicate: %1 record(s) cloned").arg(successCount));
}

CellRecord* ObjectWindowDialog::getSelectedCell() const
{
    QModelIndex index = mTreeView->currentIndex();
    if (!index.isValid() || !mModel->parent(index).isValid())
        return nullptr;

    int categoryId = mModel->getCategoryIndex(index);
    int recordIndex = mModel->getRecordIndex(index);
    CkId::Type type = static_cast<CkId::Type>(mModel->getCategoryType(categoryId));

    if (type == CkId::Type_Cel_)
    {
        auto& collection = mData->getCellCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            return &collection.getRecord(recordIndex).get();
        }
    }
    return nullptr;
}