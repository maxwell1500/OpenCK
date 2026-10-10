#include "watereditor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/collection.hpp"
#include "../../model/world/collection_impl.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/tools/editrecordcommand.hpp"
#include "../../model/tools/undostack.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "logger.hpp"

#include "../../../libs/files/esm/glob.hpp"
#include "../../../libs/files/esm/gmst.hpp"
#include "../../../libs/files/esm/esmwriter.hpp"
#include "../../../libs/files/esm/waterecord.hpp"
#include "../../../libs/files/esm/gameformat.hpp"
#include "../../model/tools/addrecordcommand.hpp"
#include "../../model/world/idtable.hpp"

#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFile>
#include <utility>

namespace {

template<typename T>
void applySettingValue(T& setting, const QString& newValue)
{
    switch (setting.value.getType())
    {
    case Var_Short:
        setting.value.setShort(static_cast<quint16>(newValue.toUInt()));
        break;
    case Var_Int:
    case Var_Long:
        setting.value.setInt(static_cast<quint32>(newValue.toUInt()));
        break;
    case Var_Float:
        setting.value.setFloat(newValue.toFloat());
        break;
    case Var_String:
    case Var_LString:
        setting.value.setString(newValue);
        break;
    case Var_Bool:
        setting.value.setBool(newValue.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0
                             || newValue == QLatin1String("1"));
        break;
    default:
        break;
    }
}
void initializeSettingValue(GameSetting& setting, const QString& raw)
{
    const QString v = raw.trimmed();
    bool ok = false;
    if (v.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0 ||
        v.compare(QStringLiteral("false"), Qt::CaseInsensitive) == 0)
    {
        setting.value.setBool(v.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0);
        return;
    }
    if (!v.contains(QLatin1Char('.')) && v.toLongLong(&ok) && ok)
    {
        setting.value.setInt(static_cast<quint32>(v.toLongLong()));
        return;
    }
    if (v.toFloat(&ok) && ok)
    {
        setting.value.setFloat(v.toFloat());
        return;
    }
    setting.value.setString(v);
}

} // namespace

namespace WaterCatalog {

struct SettingMeta {
    QString name;
    QString category;
    QString description;
};

static const QList<SettingMeta>& waterGmstCatalog(GameFormat::Game)
{
    static const QList<SettingMeta> catalog = {
        {"fWaterHeight", "Surface Level", "Global default water plane elevation"},
        {"fWaterWaveHeight", "Waves & Dynamics", "Base amplitude for procedural surface waves"},
        {"fWaterWaveSpeed", "Waves & Dynamics", "Propagation velocity for surface waves"},
        {"fWaterWaveFrequency", "Waves & Dynamics", "Spatial frequency of procedural wave ripples"},
        {"fWaterReflectance", "Optics", "Fresnel surface reflectance multiplier"},
        {"fWaterFresnel", "Optics", "Fresnel power coefficient for angle-of-incidence reflection"},
        {"fWaterSunSpecular", "Optics", "Sunlight specular reflection intensity"},
        {"fWaterSunSpecPower", "Optics", "Sun specular highlight glossiness exponent"},
        {"fWaterTransparency", "Underwater", "Water body volumetric light transmission factor"},
        {"fWaterUnderwaterFogDist", "Underwater", "Distance where underwater volumetric fog becomes opaque"},
        {"fWaterDisplacement", "Waves & Dynamics", "Geometry vertex displacement multiplier"},
        {"fWaterCaustics", "Optics", "Animated caustic light pattern projection intensity"},
        {"fWaterDepth", "Surface Level", "Maximum rendered depth for shallow-water blending"},
        {"fWaterSurfaceTension", "Waves & Dynamics", "Surface tension capillary ripple dampener"},
        {"fWaterCurrentSpeed", "Flow", "Directional fluid stream velocity"},
        {"fWaterTurbidity", "Underwater", "Suspended sediment opacity coefficient"},
        {"fOceanWaveHeight", "Waves & Dynamics", "Large-scale ocean water wave amplitude"},
        {"fOceanWaveSpeed", "Waves & Dynamics", "Large-scale ocean wave propagation speed"},
        {"fOceanWaveFrequency", "Waves & Dynamics", "Spatial frequency of deep ocean swell"},
        {"fRiverWaveHeight", "Flow", "Flowing river water surface chop amplitude"},
        {"fRiverWaveSpeed", "Flow", "Flowing river water flow speed multiplier"},
        {"fLakeWaveHeight", "Waves & Dynamics", "Enclosed lake water wave amplitude"},
        {"fWaterRefraction", "Optics", "Underwater distortion refraction magnitude"},
        {"fWaterShorelineDampening", "Flow", "Wave amplitude dampening near shoreline geometry"},
        {"bWaterReflections", "Toggles", "Enable dynamic scene reflections on water surface"},
        {"bWaterRefractions", "Toggles", "Enable screen-space optical refraction through water"},
        {"bWaterCaustics", "Toggles", "Enable animated caustic rendering on submerged surfaces"}
    };
    return catalog;
}

static const QList<SettingMeta>& waterGlobCatalog(GameFormat::Game)
{
    static const QList<SettingMeta> catalog = {
        {"WaterHeight", "Levels", "Current active cell global water level"},
        {"WaterCurrent", "Flow", "Global stream flow velocity vector magnitude"},
        {"WaterDamage", "Hazards", "Damage per second inflicted while submerged in water"},
        {"WaterRadiation", "Hazards", "Rad dose rate while swimming in water"},
        {"WaterLevel", "Levels", "Reference water plane height"},
        {"OceanLevel", "Levels", "Worldspace base ocean sea level"},
        {"LakeLevel", "Levels", "Worldspace base inland lake elevation"}
    };
    return catalog;
}

static bool isCatalogWaterGmst(const QString& name, GameFormat::Game game)
{
    for (const auto& meta : waterGmstCatalog(game)) {
        if (meta.name.compare(name, Qt::CaseInsensitive) == 0)
            return true;
    }
    return name.startsWith("fWater", Qt::CaseInsensitive) ||
           name.startsWith("bWater", Qt::CaseInsensitive) ||
           name.startsWith("fOcean", Qt::CaseInsensitive) ||
           name.startsWith("fRiver", Qt::CaseInsensitive) ||
           name.startsWith("fLake", Qt::CaseInsensitive);
}

static bool isCatalogWaterGlob(const QString& name, GameFormat::Game game)
{
    for (const auto& meta : waterGlobCatalog(game)) {
        if (meta.name.compare(name, Qt::CaseInsensitive) == 0)
            return true;
    }
    return name.startsWith("Water", Qt::CaseInsensitive) ||
           name.startsWith("Ocean", Qt::CaseInsensitive) ||
           name.startsWith("River", Qt::CaseInsensitive) ||
           name.startsWith("Lake", Qt::CaseInsensitive);
}

static QString getSettingDescription(const QString& name, GameFormat::Game game, bool isGlob)
{
    const auto& catalog = isGlob ? waterGlobCatalog(game) : waterGmstCatalog(game);
    for (const auto& meta : catalog) {
        if (meta.name.compare(name, Qt::CaseInsensitive) == 0)
            return QString("[%1] %2").arg(meta.category, meta.description);
    }
    return isGlob ? QStringLiteral("Global variable controlling water level, flow, or hazards.")
                  : QStringLiteral("Game setting controlling water simulation, physics, or rendering.");
}

} // namespace WaterCatalog

WaterEditor::WaterEditor(Data* data, std::function<bool()> saveCallback,
                         QWidget* parent)
    : QDialog(parent),
      mData(data),
      mSaveCallback(std::move(saveCallback)),
      mTree(nullptr),
      mDetailEdit(nullptr),
      mAddSettingButton(nullptr),
      mEditButton(nullptr),
      mDeleteButton(nullptr),
      mSaveButton(nullptr),
      mStatusLabel(nullptr),
      mSelectedName(),
      mSelectedValue(),
      mSelectedType()
{
    LOG_INFO("WaterEditor created");
    setupUI();
    loadSettings();
}

WaterEditor::~WaterEditor()
{
}

void WaterEditor::setupUI()
{
    setWindowTitle("Water Editor");
    setMinimumSize(1200, 800);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    auto* topBar = new QHBoxLayout();
    QLineEdit* searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Search water settings...");
    topBar->addWidget(new QLabel("Search:"));
    topBar->addWidget(searchEdit, 1);
    mainLayout->addLayout(topBar);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    mTree = new QTreeWidget();
    mTree->setHeaderLabels(QStringList() << "Setting" << "Type" << "Value");
    mTree->setColumnWidth(0, 350);
    mTree->setColumnWidth(1, 100);
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
    mAddSettingButton = new QPushButton(tr("Add Item"));
    buttonBar->addWidget(mAddSettingButton);

    mEditButton = new QPushButton("Edit");
    mEditButton->setEnabled(false);
    buttonBar->addWidget(mEditButton);

    mDeleteButton = new QPushButton("Delete");
    mDeleteButton->setEnabled(false);
    buttonBar->addWidget(mDeleteButton);

    buttonBar->addStretch();

    mSaveButton = new QPushButton(tr("Save Changes"));
    buttonBar->addWidget(mSaveButton);

    mainLayout->addLayout(buttonBar);

    mStatusLabel = new QLabel("Ready");
    mainLayout->addWidget(mStatusLabel);

    connect(mTree, &QTreeWidget::itemClicked, this, &WaterEditor::onNodeSelected);
    connect(mAddSettingButton, &QPushButton::clicked, this, &WaterEditor::onAddSetting);
    connect(mEditButton, &QPushButton::clicked, this, &WaterEditor::onEditSetting);
    connect(mDeleteButton, &QPushButton::clicked, this, &WaterEditor::onDeleteSetting);
    connect(mSaveButton, &QPushButton::clicked, this, &WaterEditor::onSave);
}

void WaterEditor::loadSettings()
{
    mTree->clear();
    mSelectedName.clear();
    mSelectedValue.clear();
    mSelectedType.clear();
    mSelectedFormId = 0;

    GameFormat::Game game = mData ? mData->currentGame() : GameFormat::Game::Unknown;

    // 1. Water Records (WATR)
    auto& wateCollection = mData->getWateCollection();
    auto wateRecords = wateCollection.getRecords();
    QTreeWidgetItem* waterRecGroup = new QTreeWidgetItem(mTree);
    waterRecGroup->setText(0, "Water Records (WATR)");
    waterRecGroup->setText(1, "GROUP");
    waterRecGroup->setText(2, QString("%1 water records").arg(wateRecords.size()));

    for (const auto& record : wateRecords) {
        if (record.state == State_Erased) continue;
        const WateRecord& watr = record.get();
        QTreeWidgetItem* item = new QTreeWidgetItem(waterRecGroup);
        item->setText(0, watr.editorId.isEmpty() ? QString("Water_%1").arg(watr.formId, 8, 16, QChar('0')).toUpper() : watr.editorId);
        item->setText(1, "WATR");
        item->setText(2, QString("Name: %1 | Waves: %2 | Damage: %3")
            .arg(watr.fullName)
            .arg(watr.waveHeight, 0, 'f', 2)
            .arg(watr.damage, 0, 'f', 1));
        item->setData(0, Qt::UserRole, QStringLiteral("WATR"));
        item->setData(0, Qt::UserRole + 1, watr.editorId);
        item->setData(0, Qt::UserRole + 2, watr.formId);
    }

    // 2. Global Variables (GLOB) - water related
    auto& globCollection = mData->getGlobCollection();
    auto globRecords = globCollection.getRecords();

    QTreeWidgetItem* globGroup = new QTreeWidgetItem(mTree);
    globGroup->setText(0, "Water Global Variables (GLOB)");
    globGroup->setText(1, "GROUP");

    int globCount = 0;
    for (const auto& record : globRecords) {
        if (record.state == State_Erased) continue;

        const GlobalVariable& glob = record.get();
        QString name = glob.editorId;
        QString value;

        const Variant& var = glob.value;
        QVariant qvar = var.getData();
        if (qvar.type() == QVariant::Int || qvar.type() == QVariant::LongLong) {
            value = QString::number(qvar.toLongLong());
        } else if (qvar.type() == QVariant::Double) {
            value = QString::number(qvar.toDouble(), 'f', 2);
        } else if (qvar.type() == QVariant::String) {
            value = qvar.toString();
        } else if (qvar.type() == QVariant::Bool) {
            value = qvar.toBool() ? "true" : "false";
        }

        if (WaterCatalog::isCatalogWaterGlob(name, game)) {
            QTreeWidgetItem* item = new QTreeWidgetItem(globGroup);
            item->setText(0, name);
            item->setText(1, "GLOB");
            item->setText(2, value);
            item->setData(0, Qt::UserRole, QStringLiteral("GLOB"));
            item->setData(0, Qt::UserRole + 1, name);
            item->setData(0, Qt::UserRole + 2, value);
            globCount++;
        }
    }
    globGroup->setText(2, QString("Water Globals (%1 settings)").arg(globCount));

    // 3. Game Settings (GMST) - water related
    auto& gmstCollection = mData->getGameSettings();
    auto gmstRecords = gmstCollection.getRecords();

    QTreeWidgetItem* gmstGroup = new QTreeWidgetItem(mTree);
    gmstGroup->setText(0, "Water Game Settings (GMST)");
    gmstGroup->setText(1, "GROUP");

    int gmstCount = 0;
    for (const auto& record : gmstRecords) {
        if (record.state == State_Erased) continue;

        const GameSetting& gmst = record.get();
        QString name = gmst.editorId;
        QString value;

        const Variant& var = gmst.value;
        QVariant qvar = var.getData();
        if (qvar.type() == QVariant::Int || qvar.type() == QVariant::LongLong) {
            value = QString::number(qvar.toLongLong());
        } else if (qvar.type() == QVariant::Double) {
            value = QString::number(qvar.toDouble(), 'f', 2);
        } else if (qvar.type() == QVariant::String) {
            value = qvar.toString();
        } else if (qvar.type() == QVariant::Bool) {
            value = qvar.toBool() ? "true" : "false";
        }

        if (WaterCatalog::isCatalogWaterGmst(name, game)) {
            QTreeWidgetItem* item = new QTreeWidgetItem(gmstGroup);
            item->setText(0, name);
            item->setText(1, "GMST");
            item->setText(2, value);
            item->setData(0, Qt::UserRole, QStringLiteral("GMST"));
            item->setData(0, Qt::UserRole + 1, name);
            item->setData(0, Qt::UserRole + 2, value);
            gmstCount++;
        }
    }
    gmstGroup->setText(2, QString("Water Game Settings (%1 settings)").arg(gmstCount));

    mTree->expandAll();
    mStatusLabel->setText(QString("Loaded %1 WATR, %2 globals, %3 game settings")
        .arg(wateRecords.size()).arg(globCount).arg(gmstCount));
    LOG_INFO(QString("Loaded %1 WATR, %2 water GLOB, %3 water GMST")
        .arg(wateRecords.size()).arg(globCount).arg(gmstCount));
}

void WaterEditor::refreshTree()
{
    loadSettings();
}

void WaterEditor::onNodeSelected(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    if (!item) return;

    QString type = item->data(0, Qt::UserRole).toString();
    if (type.isEmpty())
    {
        mEditButton->setEnabled(false);
        mDeleteButton->setEnabled(false);
        mSelectedType.clear();
        mSelectedName.clear();
        mSelectedValue.clear();
        mSelectedFormId = 0;
        return;
    }

    mEditButton->setEnabled(true);
    mDeleteButton->setEnabled(true);

    mSelectedType = type;
    mSelectedName = item->data(0, Qt::UserRole + 1).toString();

    if (type == QLatin1String("WATR"))
    {
        mSelectedFormId = item->data(0, Qt::UserRole + 2).toUInt();
        auto& coll = mData->getWateCollection();
        int idx = coll.searchId(mSelectedName);
        if (idx >= 0)
        {
            const auto& watr = coll.getRecord(idx).get();
            QString text;
            text += QString("<h2>Water: %1</h2>").arg(watr.editorId);
            text += QString("<p><b>FormID:</b> 0x%1</p>").arg(watr.formId, 8, 16, QChar('0')).toUpper();
            text += QString("<p><b>Full Name:</b> %1</p>").arg(watr.fullName);
            text += QString("<p><b>Wave Height:</b> %1</p>").arg(watr.waveHeight);
            text += QString("<p><b>Wind Velocity:</b> %1</p>").arg(watr.windVel);
            text += QString("<p><b>Damage:</b> %1</p>").arg(watr.damage);
            text += QString("<p><b>Color:</b> 0x%1</p>").arg(watr.color, 6, 16, QChar('0')).toUpper();
            text += QString("<p><b>Flags:</b> 0x%1</p>").arg(watr.waterFlags, 8, 16, QChar('0')).toUpper();
            text += "<hr><p><b>Type:</b> Water Record (WATR)</p>";
            mDetailEdit->setHtml(text);
        }
    }
    else
    {
        mSelectedValue = item->data(0, Qt::UserRole + 2).toString();
        mSelectedFormId = 0;
        showSettingDetails(mSelectedName, mSelectedValue);
    }
}

void WaterEditor::showSettingDetails(const QString& name, const QString& value)
{
    GameFormat::Game game = mData ? mData->currentGame() : GameFormat::Game::Unknown;
    bool isGlob = mSelectedType == QLatin1String("GLOB");
    QString desc = WaterCatalog::getSettingDescription(name, game, isGlob);

    QString text;
    text += QString("<h2>%1</h2>").arg(name);
    text += QString("<p><b>Current Value:</b> %1</p>").arg(value);
    text += "<hr>";
    text += "<p><b>Description:</b></p>";
    text += QString("<p>%1</p>").arg(desc);
    text += QString("<p><b>Type:</b> %1</p>").arg(isGlob ? "GlobalVariable (GLOB)" : "GameSetting (GMST)");

    mDetailEdit->setHtml(text);
}

void WaterEditor::onAddSetting()
{
    QStringList options;
    options << tr("Water Record (WATR)")
            << tr("Water Game Setting (GMST)")
            << tr("Water Global Variable (GLOB)");

    bool ok = false;
    QString choice = QInputDialog::getItem(this, tr("Add Item"),
        tr("Select item type to create:"), options, 0, false, &ok);
    if (!ok || choice.isEmpty()) return;

    if (choice.contains("WATR"))
    {
        QString id = QInputDialog::getText(this, tr("Add Water Record"),
            tr("Enter Editor ID for new Water:"), QLineEdit::Normal, "", &ok);
        if (!ok || id.trimmed().isEmpty()) return;
        const QString finalId = id.trimmed();

        auto& coll = mData->getWateCollection();
        if (coll.searchId(finalId) >= 0)
        {
            QMessageBox::warning(this, tr("Add Water Record"),
                tr("A water record named '%1' already exists.").arg(finalId));
            return;
        }

        WateRecord rec;
        rec.blank();
        rec.editorId = finalId;
        rec.fullName = finalId;
        try {
            rec.formId = mData->createNewRecord(CkId::Type_Wate_, finalId);
        } catch (const std::exception& e) {
            QMessageBox::warning(this, tr("Add Water Record"),
                tr("Could not allocate a FormID: %1").arg(QString::fromUtf8(e.what())));
            return;
        }

        auto* table = qobject_cast<IdTable*>(mData->getTableModel(CkId::Type_Wate_));
        const int appendIdx = coll.getAppendIndex(finalId, CkId::Type_Wate_);
        Record<WateRecord> record(State_ModifiedOnly, nullptr, &rec);
        if (mData->getUndoStack() && table)
        {
            mData->getUndoStack()->push(new AddRecordCommand(
                table, &coll, appendIdx, record,
                QStringLiteral("Add Water Record: %1").arg(finalId)));
        }
        else
        {
            coll.appendRecord(record, CkId::Type_Wate_);
        }
        LOG_INFO(QString("Added water record '%1'").arg(finalId));
        mStatusLabel->setText(QString("Added water '%1'").arg(finalId));
        refreshTree();
    }
    else if (choice.contains("GLOB"))
    {
        QString name = QInputDialog::getText(this, tr("Add Global Variable"),
            tr("Enter global name:"), QLineEdit::Normal, "WaterCustom", &ok);
        if (!ok || name.trimmed().isEmpty()) return;
        const QString finalId = name.trimmed();

        float val = static_cast<float>(QInputDialog::getDouble(this, tr("Set Value"),
            tr("Enter initial float value:"), 0.0, -100000.0, 100000.0, 2, &ok));
        if (!ok) return;

        auto& coll = mData->getGlobCollection();
        if (coll.searchId(finalId) >= 0)
        {
            QMessageBox::warning(this, tr("Add Global Variable"),
                tr("A global variable named '%1' already exists.").arg(finalId));
            return;
        }

        GlobalVariable glob;
        glob.blank();
        glob.editorId = finalId;
        glob.value.setFloat(val);
        try {
            glob.formId = mData->createNewRecord(CkId::Type_Glob_, finalId);
        } catch (const std::exception&) {
            glob.formId = 0;
        }

        const int indexInColl = coll.getAppendIndex(finalId, CkId::Type_Glob_);
        Record<GlobalVariable> rec(State_ModifiedOnly, nullptr, &glob);
        IdTable* table = qobject_cast<IdTable*>(mData->getTableModel(CkId::Type_Glob_));
        if (mData->getUndoStack() && table)
        {
            mData->getUndoStack()->push(new AddRecordCommand(
                table, &coll, indexInColl, rec,
                QStringLiteral("Add Global Variable: %1").arg(finalId)));
        }
        else
        {
            coll.appendRecord(rec, CkId::Type_Glob_);
        }
        LOG_INFO(QString("Added global '%1'").arg(finalId));
        mStatusLabel->setText(QString("Added global '%1'").arg(finalId));
        refreshTree();
    }
    else // GMST
    {
        QString name = QInputDialog::getText(this, tr("Add Setting"),
            tr("Enter setting name:"), QLineEdit::Normal, "fWaterCustom", &ok);
        if (!ok || name.trimmed().isEmpty()) return;
        const QString finalId = name.trimmed();

        QString value = QInputDialog::getText(this, tr("Set Value"),
            tr("Enter initial value:"), QLineEdit::Normal, "0.0", &ok);
        if (!ok) return;

        auto& coll = mData->getGameSettings();
        if (coll.searchId(finalId) >= 0)
        {
            QMessageBox::warning(this, tr("Add Setting"),
                tr("A game setting named '%1' already exists.").arg(finalId));
            return;
        }

        GameSetting gs;
        gs.blank();
        gs.editorId = finalId;
        try {
            gs.formId = mData->createNewRecord(CkId::Type_Gmst, finalId);
        } catch (const std::exception&) {
            gs.formId = 0;
        }
        initializeSettingValue(gs, value);

        const int indexInColl = coll.getAppendIndex(finalId, CkId::Type_Gmst);
        Record<GameSetting> rec(State_ModifiedOnly, nullptr, &gs);
        IdTable* table = qobject_cast<IdTable*>(mData->getTableModel(CkId::Type_Gmst));
        if (mData->getUndoStack() && table)
        {
            mData->getUndoStack()->push(new AddRecordCommand(
                table, &coll, indexInColl, rec,
                QStringLiteral("Add Game Setting: %1").arg(finalId)));
        }
        else
        {
            coll.appendRecord(rec, CkId::Type_Gmst);
        }
        LOG_INFO(QString("Added setting '%1'").arg(finalId));
        mStatusLabel->setText(QString("Added setting '%1'").arg(finalId));
        refreshTree();
    }
}

void WaterEditor::onEditSetting()
{
    if (mSelectedName.isEmpty()) return;

    if (mSelectedType == QLatin1String("WATR"))
    {
        auto& coll = mData->getWateCollection();
        int idx = coll.searchId(mSelectedName);
        if (idx < 0) return;

        WateRecord original = coll.getRecord(idx).get();
        bool ok = false;
        double newHeight = QInputDialog::getDouble(this, tr("Edit Water"),
            tr("Enter wave height for '%1':").arg(mSelectedName),
            static_cast<double>(original.waveHeight), 0.0, 1000.0, 2, &ok);
        if (!ok) return;

        WateRecord edited = original;
        edited.waveHeight = static_cast<float>(newHeight);
        if (mData->getUndoStack())
        {
            auto* cmd = new EditRecordCommand<WateRecord>(
                &coll, idx, original, edited,
                QStringLiteral("Edit Water: %1").arg(mSelectedName));
            cmd && !(original == edited) ? mData->getUndoStack()->push(cmd) : delete cmd;
        }
        else
        {
            coll.getRecord(idx).get() = edited;
            coll.getRecord(idx).state = State_Modified;
        }
        LOG_INFO(QString("Updated water '%1'").arg(mSelectedName));
        refreshTree();
    }
    else if (mSelectedType == QLatin1String("GLOB"))
    {
        auto& coll = mData->getGlobCollection();
        int idx = coll.searchId(mSelectedName);
        if (idx < 0)
        {
            LOG_WARNING(QString("Cannot edit global '%1': not found in globals").arg(mSelectedName));
            return;
        }

        bool ok = false;
        QString newValue = QInputDialog::getText(this, tr("Edit Setting"),
            tr("Enter new value for '%1':").arg(mSelectedName),
            QLineEdit::Normal, mSelectedValue, &ok);
        if (!ok) return;

        GlobalVariable original = coll.getRecord(idx).get();
        GlobalVariable edited = original;
        applySettingValue(edited, newValue);

        if (mData->getUndoStack())
        {
            auto* cmd = new EditRecordCommand<GlobalVariable>(
                &coll, idx, original, edited,
                QStringLiteral("Edit Global Variable: %1").arg(mSelectedName));
            cmd && !(original == edited) ? mData->getUndoStack()->push(cmd) : delete cmd;
        }
        else
        {
            coll.getRecord(idx).get() = edited;
            coll.getRecord(idx).state = State_Modified;
        }
        mSelectedValue = newValue;
        LOG_INFO(QString("Updated global '%1' to '%2'").arg(mSelectedName, newValue));
        refreshTree();
    }
    else // GMST
    {
        auto& coll = mData->getGameSettings();
        int idx = coll.searchId(mSelectedName);
        if (idx < 0)
        {
            LOG_WARNING(QString("Cannot edit setting '%1': not found in game settings").arg(mSelectedName));
            return;
        }

        bool ok = false;
        QString newValue = QInputDialog::getText(this, tr("Edit Setting"),
            tr("Enter new value for '%1':").arg(mSelectedName),
            QLineEdit::Normal, mSelectedValue, &ok);
        if (!ok) return;

        GameSetting original = coll.getRecord(idx).get();
        GameSetting edited = original;
        applySettingValue(edited, newValue);

        if (mData->getUndoStack())
        {
            auto* cmd = new EditRecordCommand<GameSetting>(
                &coll, idx, original, edited,
                QStringLiteral("Edit Game Setting: %1").arg(mSelectedName));
            cmd && !(original == edited) ? mData->getUndoStack()->push(cmd) : delete cmd;
        }
        else
        {
            coll.getRecord(idx).get() = edited;
            coll.getRecord(idx).state = State_Modified;
        }
        mSelectedValue = newValue;
        LOG_INFO(QString("Updated setting '%1' to '%2'").arg(mSelectedName, newValue));
        refreshTree();
    }
}

void WaterEditor::onDeleteSetting()
{
    if (mSelectedName.isEmpty()) return;

    auto reply = QMessageBox::question(this, tr("Delete Item"),
        tr("Are you sure you want to delete '%1'?\n\nThis action cannot be undone.").arg(mSelectedName),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (mSelectedType == QLatin1String("WATR"))
        {
            mData->getWateCollection().removeRecordWithUndo(mSelectedName, mData->getUndoStack());
            LOG_INFO(QString("Deleted water record '%1'").arg(mSelectedName));
        }
        else if (mSelectedType == QLatin1String("GLOB"))
        {
            mData->getGlobCollection().removeRecordWithUndo(mSelectedName, mData->getUndoStack());
            LOG_INFO(QString("Deleted global '%1'").arg(mSelectedName));
        }
        else
        {
            mData->getGameSettings().removeRecordWithUndo(mSelectedName, mData->getUndoStack());
            LOG_INFO(QString("Deleted setting '%1'").arg(mSelectedName));
        }
        mSelectedName.clear();
        mSelectedValue.clear();
        mSelectedType.clear();
        mSelectedFormId = 0;
        refreshTree();
    }
}

void WaterEditor::onSave()
{
    if (!mSaveCallback || !mSaveCallback())
    {
        QMessageBox::warning(this, tr("Save"), tr("The active document could not be saved."));
        return;
    }
    mStatusLabel->setText(tr("Active document saved."));
}
