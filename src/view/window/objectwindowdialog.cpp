#include "objectwindowdialog.hpp"

#include "../../model/window/objectwindow.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/collection.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/tools/editrecordcommand.hpp"
#include "../../model/tools/undostack.hpp"
#include "../../view/messageboxhelper.hpp"
#include "qtformdialogmanager.hpp"
#include "referencebatchdialog.hpp"
#include "npcrecorddatawidget.hpp"
#include "racedatawidget.hpp"
#include "wthrdatawidget.hpp"
#include "sounddatawidget.hpp"
#include "classdatawidget.hpp"
#include "celldatawidget.hpp"
#include "dialdatawidget.hpp"
#include "infodatawidget.hpp"
#include "questdatawidget.hpp"
#include "globvar_editor.hpp"
#include "creatureeditor.hpp"
#include "packdatawidget.hpp"
#include "worldspacedatawidget.hpp"
#include "locationdatawidget.hpp"
#include "scenetimelinewidget.hpp"
#include "navmesheditordialog.hpp"
#include "rawsubrecordwidget.hpp"
#include "../../../libs/files/esm/effectshaderrecord.hpp"
#include "../../../libs/files/esm/imagespacerecord.hpp"
#include "../../../libs/files/esm/scenrecord.hpp"
#include "lcrteditor.hpp"
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
#include "../../../libs/files/esm/cellreferencedata.hpp"
#include "../../../libs/files/esm/sounrecord.hpp"
#include "../../../libs/files/esm/wthrrecord.hpp"
#include "../../../libs/files/esm/materialrecord.hpp"
#include "../../../libs/files/esm/creaturerecord.hpp"
#include "../../../libs/files/esm/npcrecord.hpp"
#include "../../../libs/files/esm/weaprecord.hpp"
#include "../../../libs/files/esm/armorrecord.hpp"
#include "../../../libs/files/esm/spellrecord.hpp"
#include "../../../libs/files/esm/magicrecord.hpp"
#include "../../../libs/files/esm/questrecord.hpp"
#include "../../../libs/files/esm/dialrecord.hpp"
#include "../../../libs/files/esm/inforecord.hpp"
#include "../../../libs/files/esm/treerecord.hpp"
#include "../../../libs/files/esm/alchrecord.hpp"
#include "../../../libs/files/esm/ingrrecord.hpp"
#include "../../../libs/files/esm/bookrecord.hpp"
#include "../../../libs/files/esm/miscrecord.hpp"
#include "../../../libs/files/esm/contrecord.hpp"
#include "../../../libs/files/esm/enchrecord.hpp"
#include "../../../libs/files/esm/actirecord.hpp"
#include "../../../libs/files/esm/statrecord.hpp"
#include "../../../libs/files/esm/racerecord.hpp"
#include "../../../libs/files/esm/ltexrecord.hpp"
#include "../../../libs/files/esm/landrecord.hpp"
#include "../../../libs/files/esm/ammorecord.hpp"
#include "../../../libs/files/esm/aniorecord.hpp"
#include "../../../libs/files/esm/apparatusrecord.hpp"
#include "../../../libs/files/esm/actorvalueinforecord.hpp"#include "../../../libs/files/esm/birthsignrecord.hpp"
#include "../../../libs/files/esm/climaterecord.hpp"
#include "../../../libs/files/esm/clothrecord.hpp"
#include "../../../libs/files/esm/constructibleobjectrecord.hpp"
#include "../../../libs/files/esm/combatstylerecord.hpp"
#include "../../../libs/files/esm/doorrecord.hpp"
#include "../../../libs/files/esm/effectshaderrecord.hpp"
#include "../../../libs/files/esm/explosionrecord.hpp"
#include "../../../libs/files/esm/eyesrecord.hpp"
#include "../../../libs/files/esm/florrecord.hpp"
#include "../../../libs/files/esm/formlistrecord.hpp"
#include "../../../libs/files/esm/furnrecord.hpp"
#include "../../../libs/files/esm/grassrecord.hpp"
#include "../../../libs/files/esm/hairrecord.hpp"
#include "../../../libs/files/esm/idleanimationrecord.hpp"
#include "../../../libs/files/esm/idlemarkerrecord.hpp"
#include "../../../libs/files/esm/imagespacerecord.hpp"
#include "../../../libs/files/esm/keymrecord.hpp"
#include "../../../libs/files/esm/keywordrecord.hpp"
#include "../../../libs/files/esm/lighrecord.hpp"
#include "../../../libs/files/esm/loadscreenrecord.hpp"
#include "../../../libs/files/esm/lvlcreaturerecord.hpp"
#include "../../../libs/files/esm/lvlistrecord.hpp"
#include "../../../libs/files/esm/lvspellrecord.hpp"
#include "../../../libs/files/esm/messagerecord.hpp"
#include "../../../libs/files/esm/msttrecord.hpp"
#include "../../../libs/files/esm/navmrecord.hpp"
#include "../../../libs/files/esm/noterecord.hpp"
#include "../../../libs/files/esm/outfitrecord.hpp"
#include "../../../libs/files/esm/projectilerecord.hpp"
#include "../../../libs/files/esm/regionrecord.hpp"
#include "../../../libs/files/esm/roadrecord.hpp"
#include "../../../libs/files/esm/scriptrecord.hpp"
#include "../../../libs/files/esm/scrollrecord.hpp"
#include "../../../libs/files/esm/shaderparticlerecord.hpp"
#include "../../../libs/files/esm/slgmrecord.hpp"
#include "../../../libs/files/esm/soundmarkerrecord.hpp"
#include "../../../libs/files/esm/staticcollectionrecord.hpp"
#include "../../../libs/files/esm/texturesetrecord.hpp"
#include "../../../libs/files/esm/waterecord.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QInputDialog>
#include <QSettings>
#include <QFileDialog>

namespace {
template <typename T>
bool tryResolveComponents(BaseCollection* coll, int recordIndex,
                          openck::FormComponents*& components, void*& recordPtr)
{
    auto* typed = dynamic_cast<Collection<T>*>(coll);
    if (!typed)
        return false;
    if (recordIndex < 0 || recordIndex >= typed->size())
        return false;
    auto& record = typed->getRecord(recordIndex).get();
    components = &record.components;
    recordPtr = &record;
    return true;
}

#define FOR_EACH_COMPONENT_RECORD_TYPE(MACRO) \
    MACRO(ActiRecord) \
    MACRO(ActorValueInfoRecord) \
    MACRO(AlchRecord) \
    MACRO(AmmoRecord) \
    MACRO(AnioRecord) \
    MACRO(AppaRecord) \
    MACRO(ArmorRecord) \
    MACRO(BookRecord) \
    MACRO(BsgnRecord) \
    MACRO(CellRecord) \
    MACRO(ClassRecord) \
    MACRO(ClimateRecord) \
    MACRO(ClotRecord) \
    MACRO(CobjRecord) \
    MACRO(ContRecord) \
    MACRO(CreatureRecord) \
    MACRO(CstyRecord) \
    MACRO(DialRecord) \
    MACRO(DoorRecord) \
    MACRO(EfshRecord) \
    MACRO(EnchRecord) \
    MACRO(ExplRecord) \
    MACRO(EyesRecord) \
    MACRO(FactRecord) \
    MACRO(FlorRecord) \
    MACRO(FormListRecord) \
    MACRO(FurnRecord) \
    MACRO(GrassRecord) \
    MACRO(HairRecord) \
    MACRO(IdleAnimationRecord) \
    MACRO(IdleMarkerRecord) \
    MACRO(ImgsRecord) \
    MACRO(InfoRecord) \
    MACRO(IngrRecord) \
    MACRO(KeymRecord) \
    MACRO(KeywordRecord) \
    MACRO(LandRecord) \
    MACRO(LighRecord) \
    MACRO(LoadScreenRecord) \
    MACRO(LocationRecord) \
    MACRO(LtexRecord) \
    MACRO(LvlcRecord) \
    MACRO(LvliRecord) \
    MACRO(LvspRecord) \
    MACRO(MagicRecord) \
    MACRO(MaterialRecord) \
    MACRO(MesgRecord) \
    MACRO(MiscRecord) \
    MACRO(MsttRecord) \
    MACRO(NavmRecord) \
    MACRO(NpcRecord) \
    MACRO(NoteRecord) \
    MACRO(OutfitRecord) \
    MACRO(PackageRecord) \
    MACRO(PerkRecord) \
    MACRO(ProjRecord) \
    MACRO(QuestRecord) \
    MACRO(RaceRecord) \
    MACRO(RefrRecord) \
    MACRO(RegionRecord) \
    MACRO(RoadRecord) \
    MACRO(ScriptRecord) \
    MACRO(ScrRecord) \
    MACRO(SlgmRecord) \
    MACRO(SmqnRecord) \
    MACRO(SounRecord) \
    MACRO(SpellRecord) \
    MACRO(SpgdRecord) \
    MACRO(StaticCollectionRecord) \
    MACRO(StatRecord) \
    MACRO(ScenRecord) \
    MACRO(TextureSetRecord) \
    MACRO(TreeRecord) \
    MACRO(WateRecord) \
    MACRO(WeaponRecord) \
    MACRO(WorldspaceRecord) \
    MACRO(WthrRecord)

bool resolveComponents(BaseCollection* coll, int recordIndex,
                       openck::FormComponents*& components, void*& recordPtr)
{
#define RESOLVE_RECORD_TYPE(recType) \
    if (tryResolveComponents<recType>(coll, recordIndex, components, recordPtr)) return true;
    FOR_EACH_COMPONENT_RECORD_TYPE(RESOLVE_RECORD_TYPE)
#undef RESOLVE_RECORD_TYPE
#undef FOR_EACH_COMPONENT_RECORD_TYPE
    return false;
}
} // namespace

ObjectWindowDialog::ObjectWindowDialog(Data* data, QWidget* parent)
    : QDockWidget(parent),
      mData(data),
      mModel(nullptr),
      mTreeView(nullptr),
      mFilterEdit(nullptr),
      mSavedFilterCombo(nullptr),
      mColumnLayoutCombo(nullptr),
      mEditButton(nullptr),
      mDeleteButton(nullptr),
      mCloneButton(nullptr),
      mStatusLabel(nullptr),
      mContextMenu(nullptr)
{
    setWindowTitle("Object Window");
    setupUI();

    static bool factoriesRegistered = false;
    if (!factoriesRegistered)
    {
        factoriesRegistered = true;
        using namespace openck;
        QtFormDialogManager::instance().registerFactory(
            QStringLiteral("NPC_"),
            [](FormComponents* comps, void* recPtr, QWidget* parent) -> QWidget* {
                return new NpcRecordDataWidget(recPtr, comps, parent);
            });
        QtFormDialogManager::instance().registerFactory(
            QStringLiteral("RACE"),
            [](FormComponents* comps, void* recPtr, QWidget* parent) -> QWidget* {
                return new RaceDataWidget(recPtr, comps, parent);
            });
        QtFormDialogManager::instance().registerFactory(
            QStringLiteral("WTHR"),
            [](FormComponents* comps, void* recPtr, QWidget* parent) -> QWidget* {
                return new WthrDataWidget(recPtr, comps, parent);
            });
        QtFormDialogManager::instance().registerFactory(
            QStringLiteral("SOUN"),
            [](FormComponents* comps, void* recPtr, QWidget* parent) -> QWidget* {
                return new SoundDataWidget(recPtr, comps, parent);
            });
        QtFormDialogManager::instance().registerFactory(
            QStringLiteral("CLAS"),
            [](FormComponents* comps, void* recPtr, QWidget* parent) -> QWidget* {
                return new ClassDataWidget(recPtr, comps, parent);
            });
        QtFormDialogManager::instance().registerFactory(
            QStringLiteral("DIAL"),
            [](FormComponents* comps, void* recPtr, QWidget* parent) -> QWidget* {
                return new DialDataWidget(recPtr, comps, parent);
            });
        QtFormDialogManager::instance().registerFactory(
            QStringLiteral("INFO"),
            [](FormComponents* comps, void* recPtr, QWidget* parent) -> QWidget* {
                return new InfoDataWidget(recPtr, comps, parent);
            });
        QtFormDialogManager::instance().registerFactory(
            QStringLiteral("QUST"),
            [](FormComponents* comps, void* recPtr, QWidget* parent) -> QWidget* {
                return new QuestDataWidget(recPtr, comps, parent);
            });
        QtFormDialogManager::instance().registerFactory(
            QStringLiteral("CREA"),
            [](FormComponents* comps, void* recPtr, QWidget* parent) -> QWidget* {
                return new CreatureDataWidget(recPtr, comps, parent);
            });
        QtFormDialogManager::instance().registerFactory(
            QStringLiteral("PACK"),
            [](FormComponents* comps, void* recPtr, QWidget* parent) -> QWidget* {
                return new PackDataWidget(recPtr, comps, parent);
            });
        QtFormDialogManager::instance().registerFactory(
            QStringLiteral("WRLD"),
            [](FormComponents* comps, void* recPtr, QWidget* parent) -> QWidget* {
                return new WorldspaceDataWidget(recPtr, comps, parent);
            });
        QtFormDialogManager::instance().registerFactory(
            QStringLiteral("LCTN"),
            [](FormComponents* comps, void* recPtr, QWidget* parent) -> QWidget* {
                return new LocationDataWidget(recPtr, comps, parent);
            });
        QtFormDialogManager::instance().registerFactory(
            QStringLiteral("SCEN"),
            [](FormComponents* comps, void* recPtr, QWidget* parent) -> QWidget* {
                Q_UNUSED(comps);
                // Scene timeline: phases live in the record's raw PHDA
                // subrecords until validated against real data, so expose a
                // phase list owned by the widget (edit-in-memory for now).
                auto* phases = new QVector<ScenePhase>();
                Q_UNUSED(recPtr);
                auto* w = new SceneTimelineWidget(phases, parent);
                w->setOwnedPhases(phases);
                return w;
            });
        QtFormDialogManager::instance().registerFactory(
            QStringLiteral("EFSH"),
            [](FormComponents*, void* recPtr, QWidget* parent) -> QWidget* {
                auto* w = new RawSubrecordWidget(parent);
                if (auto* rec = static_cast<EfshRecord*>(recPtr))
                    w->setSubrecords(rec->rawSubRecords);
                return w;
            });
        QtFormDialogManager::instance().registerFactory(
            QStringLiteral("IMGS"),
            [](FormComponents*, void* recPtr, QWidget* parent) -> QWidget* {
                auto* w = new RawSubrecordWidget(parent);
                if (auto* rec = static_cast<ImgsRecord*>(recPtr))
                    w->setSubrecords(rec->rawSubRecords);
                return w;
            });
        QtFormDialogManager::instance().registerFactory(
            QStringLiteral("SCEN"),
            [](FormComponents*, void* recPtr, QWidget* parent) -> QWidget* {
                auto* w = new RawSubrecordWidget(parent);
                if (auto* rec = static_cast<ScenRecord*>(recPtr))
                    w->setSubrecords(rec->rawSubRecords);
                return w;
            });
    }
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

    mSavedFilterCombo = new QComboBox();
    mSavedFilterCombo->setToolTip("Saved filters (user-created, persisted)");
    mSavedFilterCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    filterLayout->addWidget(mSavedFilterCombo);

    QPushButton* saveFilterButton = new QPushButton("Save");
    QPushButton* deleteFilterButton = new QPushButton("Delete");
    saveFilterButton->setToolTip("Save the current filter text under a name");
    deleteFilterButton->setToolTip("Delete the selected saved filter");
    filterLayout->addWidget(saveFilterButton);
    filterLayout->addWidget(deleteFilterButton);

    mainLayout->addLayout(filterLayout);

    connect(mFilterEdit, &QLineEdit::textChanged, mModel, &ObjectWindowModel::applyFilter);
    connect(saveFilterButton, &QPushButton::clicked, this, &ObjectWindowDialog::saveFilter);
    connect(mSavedFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &ObjectWindowDialog::loadFilter);
    connect(deleteFilterButton, &QPushButton::clicked, this, &ObjectWindowDialog::deleteSavedFilter);
    refreshSavedFilters();

    auto* layoutRow = new QHBoxLayout();
    layoutRow->addWidget(new QLabel("Layout:"));
    mColumnLayoutCombo = new QComboBox();
    mColumnLayoutCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    layoutRow->addWidget(mColumnLayoutCombo);

    QPushButton* saveLayoutButton = new QPushButton("Save");
    QPushButton* deleteLayoutButton = new QPushButton("Delete");
    saveLayoutButton->setToolTip("Save the current column widths/visibility under a name");
    deleteLayoutButton->setToolTip("Delete the selected column layout");
    layoutRow->addWidget(saveLayoutButton);
    layoutRow->addWidget(deleteLayoutButton);
    layoutRow->addStretch();
    mainLayout->addLayout(layoutRow);

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

    connect(mEditButton, &QPushButton::clicked, this, &ObjectWindowDialog::editSelected);
    connect(mDeleteButton, &QPushButton::clicked, this, &ObjectWindowDialog::deleteSelected);
    connect(mCloneButton, &QPushButton::clicked, this, &ObjectWindowDialog::cloneSelected);
    connect(saveLayoutButton, &QPushButton::clicked, this, &ObjectWindowDialog::saveColumnLayout);
    connect(mColumnLayoutCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &ObjectWindowDialog::loadColumnLayout);
    connect(deleteLayoutButton, &QPushButton::clicked, this, &ObjectWindowDialog::deleteColumnLayout);
    refreshColumnLayouts();
    connect(mTreeView, &QTreeView::doubleClicked, this, &ObjectWindowDialog::onDoubleClick);
    connect(mTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
        [this](const QModelIndex& current, const QModelIndex&) {
            if (current.isValid() && mModel->isRecord(current)) {
                int cat = mModel->getCategoryIndex(current);
                int rec = mModel->getRecordIndex(current);
                QString eid = mModel->getRecordEditorId(cat, rec);
                emit recordSelected(cat, rec, eid);
            }
        });
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

    if (!index.isValid() || mModel->isRecord(index))
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

            // Reference batch actions apply to selected REFR records.
            bool allRefr = true;
            for (const QModelIndex& idx : selectedIndices)
            {
                if (!idx.isValid() || !mModel->isRecord(idx)) { allRefr = false; break; }
                int cat = mModel->getCategoryIndex(idx);
                if (mModel->getCategoryType(cat) != static_cast<int>(CkId::Type_Refr_))
                {
                    allRefr = false;
                    break;
                }
            }
            if (allRefr)
            {
                QAction* batchRefAction = mContextMenu->addAction(
                    QString("Batch Actions on %1 References...").arg(count));
                connect(batchRefAction, &QAction::triggered, this, &ObjectWindowDialog::batchReferenceActions);
            }
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
    if (!index.isValid() || !mModel->isRecord(index))
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
            openck::QtFormDialogManager::instance().openOrFocus(
                formIdKey, QStringLiteral("NPC_"), &rec.components, &rec, this);
        }
        break;
    }
    case CkId::Type_Crea_:
    {
        auto& collection = mData->getCreatureCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            CreatureRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(
                formIdKey, QStringLiteral("CREA"), &rec.components, &rec, this);
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
            openck::QtFormDialogManager::instance().openOrFocus(
                formIdKey, QStringLiteral("QUST"), &rec.components, &rec, this);
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
            auto& record = collection.getRecord(recordIndex);
            RaceRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(
                formIdKey, QStringLiteral("RACE"), &rec.components, &rec, this);
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
            auto& record = collection.getRecord(recordIndex);
            PackageRecord& pack = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(pack.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(
                formIdKey, QStringLiteral("PACK"), &pack.components, &pack, this);
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
            openck::QtFormDialogManager::instance().openOrFocus(
                formIdKey, QStringLiteral("CLAS"), &rec.components, &rec, this);
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
            openck::QtFormDialogManager::instance().openOrFocus(
                formIdKey, QStringLiteral("WRLD"), &rec.components, &rec, this);
        }
        break;
    }
    case CkId::Type_LOCT_:
    {
        auto& collection = mData->getLocationCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            LocationRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(
                formIdKey, QStringLiteral("LCTN"), &rec.components, &rec, this);
        }
        break;
    }
    case CkId::Type_Refr_:
    {
        auto& collection = mData->getRefrCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            RefrRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(
                formIdKey, QStringLiteral("REFR"), &rec.components, &rec, this);
        }
        break;
    }
    case CkId::Type_Dial_:
    {
        auto& collection = mData->getDialCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            DialRecord& dial = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(dial.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(
                formIdKey, QStringLiteral("DIAL"), &dial.components, &dial, this);
        }
        break;
    }
    case CkId::Type_Info_:
    {
        auto& collection = mData->getInfoCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            InfoRecord& info = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(info.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(
                formIdKey, QStringLiteral("INFO"), &info.components, &info, this);
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
    case CkId::Type_Soun_:
    {
        auto& collection = mData->getSounCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            SounRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(
                formIdKey, QStringLiteral("SOUN"), &rec.components, &rec, this);
        }
        break;
    }
    case CkId::Type_Wthr_:
    {
        auto& collection = mData->getWthrCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            WthrRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(
                formIdKey, QStringLiteral("WTHR"), &rec.components, &rec, this);
        }
        break;
    }
    case CkId::Type_Navm_:
    {
        auto& collection = mData->getNavmCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            NavmRecord original = record.get();

            NavMeshData navData;
            navData.vertices = original.vertices;
            navData.triangles.reserve(original.triangles.size());
            for (const auto& t : original.triangles)
            {
                NavTriangle nt;
                nt.v0 = t.v0;
                nt.v1 = t.v1;
                nt.v2 = t.v2;
                nt.walkable = (t.flags & 1) != 0;
                if (nt.v0 >= 0 && nt.v0 < navData.vertices.size() &&
                    nt.v1 >= 0 && nt.v1 < navData.vertices.size() &&
                    nt.v2 >= 0 && nt.v2 < navData.vertices.size())
                {
                    const QVector3D a = navData.vertices[nt.v1] - navData.vertices[nt.v0];
                    const QVector3D b = navData.vertices[nt.v2] - navData.vertices[nt.v0];
                    nt.normal = QVector3D::crossProduct(a, b);
                    if (nt.normal.lengthSquared() > 0.0f)
                        nt.normal.normalize();
                }
                navData.triangles.append(nt);
            }

            NavmeshEditorDialog dialog(this);
            dialog.setNavMesh(navData);
            if (dialog.exec() == QDialog::Accepted)
            {
                const NavMeshData edited = dialog.getNavMesh();
                NavmRecord editedRecord = original;
                editedRecord.vertices = edited.vertices;
                editedRecord.triangles.clear();
                editedRecord.triangles.reserve(edited.triangles.size());
                for (const auto& nt : edited.triangles)
                {
                    NavmTriangle t;
                    t.v0 = static_cast<qint16>(nt.v0);
                    t.v1 = static_cast<qint16>(nt.v1);
                    t.v2 = static_cast<qint16>(nt.v2);
                    t.flags = nt.walkable ? 1 : 0;
                    editedRecord.triangles.append(t);
                }

                auto& coll = mData->getNavmCollection();
                int idx = coll.searchId(original.editorId);
                if (idx >= 0 && mData->getUndoStack())
                {
                    EditRecordCommand<NavmRecord>* cmd = new EditRecordCommand<NavmRecord>(
                        &coll, idx, original, editedRecord,
                        "Edit Navmesh: " + original.editorId);
                    cmd && cmd->hasChanged() ? mData->getUndoStack()->push(cmd) : delete cmd;
                }
                LOG_INFO(QString("Navmesh '%1' edited").arg(original.editorId));
            }
        }
        break;
    }
    case CkId::Type_Efsh_:
    {
        auto& collection = mData->getEfshCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            EfshRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(
                formIdKey, QStringLiteral("EFSH"), &rec.components, &rec, this);
        }
        break;
    }
    case CkId::Type_Imgs_:
    {
        auto& collection = mData->getImgsCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            ImgsRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(
                formIdKey, QStringLiteral("IMGS"), &rec.components, &rec, this);
        }
        break;
    }
    case CkId::Type_Scen_:
    {
        auto& collection = mData->getScenCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            ScenRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(
                formIdKey, QStringLiteral("SCEN"), &rec.components, &rec, this);
        }
        break;
    }
    case CkId::Type_Material_:
    {
        auto& collection = mData->getMaterialCollection();
        if (recordIndex >= 0 && recordIndex < collection.size())
        {
            auto& record = collection.getRecord(recordIndex);
            MaterialRecord& rec = record.get();
            QString formIdKey = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
            openck::QtFormDialogManager::instance().openOrFocus(formIdKey, &rec.components, this);
        }
        break;
    }
    default:
    {
        if (type != CkId::Type_None)
        {
            BaseCollection* coll = mData->getCollectionByType(type);
            if (coll && recordIndex >= 0 && recordIndex < coll->size())
            {
                openck::FormComponents* comps = nullptr;
                void* recPtr = nullptr;
                if (resolveComponents(coll, recordIndex, comps, recPtr) && comps)
                {
                    quint32 formId = coll->getFormId(recordIndex);
                    QString formIdKey = formId != 0
                        ? QStringLiteral("0x%1").arg(formId, 8, 16, QChar('0'))
                        : QStringLiteral("%1|%2").arg(editorId, QStringLiteral("0"));
                    openck::QtFormDialogManager::instance().openOrFocus(formIdKey, comps, this);
                    break;
                }
            }
        }
        QMessageBox::information(this, "Edit Record",
            QString("Record '%1' cannot be opened for editing because its data is not available.")
                .arg(editorId));
        break;
    }
    }
}
void ObjectWindowDialog::deleteSelected()
{
    QModelIndex index = mTreeView->currentIndex();
    if (!index.isValid() || !mModel->isRecord(index))
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

void ObjectWindowDialog::refreshSavedFilters()
{
    if (!mSavedFilterCombo) return;

    const int current = mSavedFilterCombo->currentIndex();
    const QString selectedName = (current >= 0)
        ? mSavedFilterCombo->itemText(current) : QString();

    mSavedFilterCombo->blockSignals(true);
    mSavedFilterCombo->clear();
    mSavedFilterCombo->addItem("");

    QSettings settings;
    const QStringList names = settings.childGroups();
    for (const QString& name : names)
    {
        if (name.startsWith(QStringLiteral("SavedFilter/")))
        {
            mSavedFilterCombo->addItem(name.mid(QStringLiteral("SavedFilter/").size()));
        }
    }

    const int restore = mSavedFilterCombo->findText(selectedName);
    mSavedFilterCombo->setCurrentIndex(restore >= 0 ? restore : 0);
    mSavedFilterCombo->blockSignals(false);
}

void ObjectWindowDialog::saveFilter()
{
    const QString text = mFilterEdit ? mFilterEdit->text() : QString();
    if (text.trimmed().isEmpty())
    {
        QMessageBox::information(this, "Save Filter",
            "Enter filter text before saving.");
        return;
    }

    bool ok = false;
    QString name = QInputDialog::getText(this, "Save Filter",
        "Filter name:", QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    name = name.trimmed();

    QSettings settings;
    settings.setValue(QStringLiteral("SavedFilter/%1/text").arg(name), text);
    settings.sync();
    refreshSavedFilters();
    if (mStatusLabel)
    {
        mStatusLabel->setText(QString("Saved filter '%1'").arg(name));
    }
    LOG_INFO(QString("Saved Object Window filter '%1'").arg(name));
}

void ObjectWindowDialog::loadFilter()
{
    const QString name = mSavedFilterCombo
        ? mSavedFilterCombo->currentText() : QString();
    if (name.isEmpty()) return;

    QSettings settings;
    const QString text = settings.value(
        QStringLiteral("SavedFilter/%1/text").arg(name)).toString();
    if (mFilterEdit && !text.isEmpty())
    {
        mFilterEdit->setText(text);
        if (mModel)
        {
            mModel->applyFilter(text);
        }
        if (mStatusLabel)
        {
            mStatusLabel->setText(QString("Loaded filter '%1'").arg(name));
        }
    }
}

void ObjectWindowDialog::deleteSavedFilter()
{
    const QString name = mSavedFilterCombo
        ? mSavedFilterCombo->currentText() : QString();
    if (name.isEmpty()) return;

    QSettings settings;
    settings.remove(QStringLiteral("SavedFilter/%1").arg(name));
    settings.sync();
    refreshSavedFilters();
    if (mStatusLabel)
    {
        mStatusLabel->setText(QString("Deleted filter '%1'").arg(name));
    }
    LOG_INFO(QString("Deleted Object Window filter '%1'").arg(name));
}

QByteArray ObjectWindowDialog::captureColumnState() const
{
    return mTreeView ? mTreeView->header()->saveState() : QByteArray();
}

void ObjectWindowDialog::applyColumnState(const QByteArray& state)
{
    if (mTreeView && !state.isEmpty())
    {
        mTreeView->header()->restoreState(state);
    }
}

void ObjectWindowDialog::refreshColumnLayouts()
{
    if (!mColumnLayoutCombo) return;

    const int current = mColumnLayoutCombo->currentIndex();
    const QString selectedName = (current >= 0)
        ? mColumnLayoutCombo->itemText(current) : QString();

    mColumnLayoutCombo->blockSignals(true);
    mColumnLayoutCombo->clear();
    mColumnLayoutCombo->addItem("");

    QSettings settings;
    const QStringList names = settings.childGroups();
    for (const QString& name : names)
    {
        if (name.startsWith(QStringLiteral("ColumnLayout/")))
        {
            mColumnLayoutCombo->addItem(name.mid(QStringLiteral("ColumnLayout/").size()));
        }
    }

    const int restore = mColumnLayoutCombo->findText(selectedName);
    mColumnLayoutCombo->setCurrentIndex(restore >= 0 ? restore : 0);
    mColumnLayoutCombo->blockSignals(false);
}

void ObjectWindowDialog::saveColumnLayout()
{
    if (!mTreeView) return;

    bool ok = false;
    QString name = QInputDialog::getText(this, "Save Column Layout",
        "Layout name:", QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    name = name.trimmed();

    QSettings settings;
    settings.setValue(QStringLiteral("ColumnLayout/%1/state").arg(name),
        captureColumnState());
    settings.sync();
    refreshColumnLayouts();
    if (mStatusLabel)
    {
        mStatusLabel->setText(QString("Saved column layout '%1'").arg(name));
    }
    LOG_INFO(QString("Saved Object Window column layout '%1'").arg(name));
}

void ObjectWindowDialog::loadColumnLayout()
{
    const QString name = mColumnLayoutCombo
        ? mColumnLayoutCombo->currentText() : QString();
    if (name.isEmpty()) return;

    QSettings settings;
    const QByteArray state = settings.value(
        QStringLiteral("ColumnLayout/%1/state").arg(name)).toByteArray();
    applyColumnState(state);
    if (mStatusLabel)
    {
        mStatusLabel->setText(QString("Loaded column layout '%1'").arg(name));
    }
}

void ObjectWindowDialog::deleteColumnLayout()
{
    const QString name = mColumnLayoutCombo
        ? mColumnLayoutCombo->currentText() : QString();
    if (name.isEmpty()) return;

    QSettings settings;
    settings.remove(QStringLiteral("ColumnLayout/%1").arg(name));
    settings.sync();
    refreshColumnLayouts();
    if (mStatusLabel)
    {
        mStatusLabel->setText(QString("Deleted column layout '%1'").arg(name));
    }
    LOG_INFO(QString("Deleted Object Window column layout '%1'").arg(name));
}

void ObjectWindowDialog::cloneSelected()
{
    QModelIndex index = mTreeView->currentIndex();
    if (!index.isValid() || !mModel->isRecord(index))
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
    if (!index.isValid() || mModel->isRecord(index))
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
    if (!index.isValid() || !mModel->isRecord(index))
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
    if (!index.isValid() || !mModel->isRecord(index))
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
    if (!index1.isValid() || !mModel->isRecord(index1))
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
    if (!index.isValid() || !mModel->isRecord(index))
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
        if (!index.isValid() || !mModel->isRecord(index))
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
        if (idx.isValid() && mModel->isRecord(idx))
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

void ObjectWindowDialog::batchReferenceActions()
{
    QList<QModelIndex> selectedIndices = getSelectedIndices();
    if (selectedIndices.isEmpty())
    {
        QMessageBox::warning(this, "No Selection",
            "Please select one or more reference records.\n\nUse Ctrl+Click for multi-select.");
        return;
    }

    const auto& collection = mData->getRefrCollection();
    const int collectionSize = collection.size();

    QVector<CellRefEntry> refs;
    QVector<int> recordIndices;
    QVector<RefrRecord> originals;

    for (const QModelIndex& idx : selectedIndices)
    {
        int categoryId = mModel->getCategoryIndex(idx);
        int recordIndex = mModel->getRecordIndex(idx);
        CkId::Type type = static_cast<CkId::Type>(mModel->getCategoryType(categoryId));
        if (type != CkId::Type_Refr_ || recordIndex < 0 || recordIndex >= collectionSize)
            continue;

        const RefrRecord& rec = collection.getRecord(recordIndex).get();

        CellRefEntry entry;
        entry.formId = rec.formId;
        entry.baseObject = rec.baseId;
        entry.posX = rec.posX;
        entry.posY = rec.posY;
        entry.posZ = rec.posZ;
        entry.rotX = rec.rotX;
        entry.rotY = rec.rotY;
        entry.rotZ = rec.rotZ;
        entry.scale = rec.scale;
        entry.flags = rec.initiallyDisabled ? 0x01 : 0;

        refs.append(entry);
        recordIndices.append(recordIndex);
        originals.append(rec);
    }

    if (refs.isEmpty())
    {
        QMessageBox::information(this, "Batch Reference Actions",
            "None of the selected records are cell references.");
        return;
    }

    ReferenceBatchDialog dialog(refs, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QVector<CellRefEntry> result = dialog.getReferences();

    int changed = 0;
    for (int i = 0; i < result.size() && i < recordIndices.size(); ++i)
    {
        const CellRefEntry& entry = result[i];
        RefrRecord& rec = originals[i];
        rec.posX = entry.posX;
        rec.posY = entry.posY;
        rec.posZ = entry.posZ;
        rec.rotX = entry.rotX;
        rec.rotY = entry.rotY;
        rec.rotZ = entry.rotZ;
        rec.scale = entry.scale;
        rec.initiallyDisabled = entry.isDisabled();

        auto& record = const_cast<IdCollection<RefrRecord>&>(collection).getRecord(recordIndices[i]);
        if (record.get() == rec)
            continue;

        if (mData->getUndoStack())
        {
            mData->getUndoStack()->push(new EditRecordCommand<RefrRecord>(
                &const_cast<IdCollection<RefrRecord>&>(collection), recordIndices[i],
                record.get(), rec, "Batch reference actions"));
        }
        ++changed;
    }

    mModel->setData(mData);
    if (mStatusLabel)
    {
        mStatusLabel->setText(QString("Batch reference actions applied to %1 record(s)").arg(changed));
    }
    LOG_INFO(QString("Batch reference actions applied to %1 reference record(s)").arg(changed));
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

ObjectWindowDialog::RecordLookupResult ObjectWindowDialog::getFormComponentsForIndex(
    int categoryId, int recordIndex) const
{
    RecordLookupResult result;
    CkId::Type type = static_cast<CkId::Type>(mModel->getCategoryType(categoryId));

    switch (type)
    {
#define CASE_RECORD(ctype, getter, recType) \
    case ctype: { \
        auto& coll = mData->getter(); \
        if (recordIndex >= 0 && recordIndex < coll.size()) { \
            auto& rec = coll.getRecord(recordIndex).get(); \
            result.components = &rec.components; \
            result.recordPtr = &rec; \
            result.recordType = #ctype; \
        } \
        break; \
    }
    CASE_RECORD(CkId::Type_Npc_, getNpcCollection, NpcRecord)
    CASE_RECORD(CkId::Type_Weap_, getWeaponCollection, WeaponRecord)
    CASE_RECORD(CkId::Type_Armor_, getArmorCollection, ArmorRecord)
    CASE_RECORD(CkId::Type_Spel_, getSpellCollection, SpellRecord)
    CASE_RECORD(CkId::Type_Magic_, getMagicCollection, MagicRecord)
    CASE_RECORD(CkId::Type_Quest_, getQuestCollection, QuestRecord)
    CASE_RECORD(CkId::Type_Dial_, getDialCollection, DialRecord)
    CASE_RECORD(CkId::Type_Info_, getInfoCollection, InfoRecord)
    CASE_RECORD(CkId::Type_Tree_, getTreeCollection, TreeRecord)
    CASE_RECORD(CkId::Type_Alch_, getAlchCollection, AlchRecord)
    CASE_RECORD(CkId::Type_Ingr_, getIngrCollection, IngrRecord)
    CASE_RECORD(CkId::Type_Book_, getBookCollection, BookRecord)
    CASE_RECORD(CkId::Type_Misc_, getMiscCollection, MiscRecord)
    CASE_RECORD(CkId::Type_Cont_, getContCollection, ContRecord)
    CASE_RECORD(CkId::Type_Ench_, getEnchCollection, EnchRecord)
    CASE_RECORD(CkId::Type_Acti_, getActiCollection, ActiRecord)
    CASE_RECORD(CkId::Type_Stat_, getStatCollection, StatRecord)
    CASE_RECORD(CkId::Type_Race_, getRaceCollection, RaceRecord)
    CASE_RECORD(CkId::Type_Class_, getClassCollection, ClassRecord)
    CASE_RECORD(CkId::Type_Fact_, getFactCollection, FactRecord)
    CASE_RECORD(CkId::Type_PerK_, getPerkCollection, PerkRecord)
    CASE_RECORD(CkId::Type_Soun_, getSounCollection, SounRecord)
    CASE_RECORD(CkId::Type_Wthr_, getWthrCollection, WthrRecord)
    CASE_RECORD(CkId::Type_Ltex_, getLtexCollection, LtexRecord)
    CASE_RECORD(CkId::Type_Cel_, getCellCollection, CellRecord)
    CASE_RECORD(CkId::Type_WRLD_, getWorldspaceCollection, WorldspaceRecord)
    CASE_RECORD(CkId::Type_LOCT_, getLocationCollection, LocationRecord)
    CASE_RECORD(CkId::Type_Refr_, getRefrCollection, RefrRecord)
    CASE_RECORD(CkId::Type_Land_, getLandCollection, LandRecord)
    CASE_RECORD(CkId::Type_Pack_, getPackCollection, PackRecord)
    CASE_RECORD(CkId::Type_Material_, getMaterialCollection, MaterialRecord)
#undef CASE_RECORD
    default:
    {
        if (type == CkId::Type_None)
            break;
        BaseCollection* coll = mData->getCollectionByType(type);
        if (!coll || recordIndex < 0 || recordIndex >= coll->size())
            break;
        openck::FormComponents* comps = nullptr;
        void* recPtr = nullptr;
        if (resolveComponents(coll, recordIndex, comps, recPtr) && comps)
        {
            result.components = comps;
            result.recordPtr = recPtr;
            result.recordType = CkId(type).getTypeName();
        }
        break;
    }
    }
    return result;
}