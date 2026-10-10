#include "weatherlighteditor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/collection.hpp"
#include "../../model/world/collection_impl.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/tools/editrecordcommand.hpp"
#include "../../model/tools/addrecordcommand.hpp"
#include "../../model/tools/undostack.hpp"
#include "../../model/world/idtable.hpp"
#include "logger.hpp"

#include "../../../libs/files/esm/gmst.hpp"
#include "../../../libs/files/esm/glob.hpp"
#include "../../../libs/files/esm/esmwriter.hpp"
#include "../../../libs/files/esm/wthrrecord.hpp"
#include "../../../libs/files/esm/lighrecord.hpp"
#include "../../../libs/files/esm/gameformat.hpp"

#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QFile>
#include <utility>

namespace {

void applySettingValue(GameSetting& setting, const QString& newValue)
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

namespace WeatherLightCatalog {

struct SettingMeta {
    QString name;
    QString category;
    QString description;
};

static const QList<SettingMeta>& weatherCatalog(GameFormat::Game game)
{
    static const QList<SettingMeta> skyrimWeather = {
        {"fWeatherTransitionTime", "Transitions", "Seconds for full weather transition"},
        {"fWeatherDistance", "Transitions", "Distance threshold for localized weather transitions"},
        {"fWeatherInterpolateTime", "Transitions", "Interpolation step duration during transition"},
        {"fWeatherTransitionFadeTime", "Transitions", "Fade blend time between sky states"},
        {"fWeatherMinTransitionTime", "Transitions", "Minimum transition duration"},
        {"fWeatherSkyChangeTime", "Sky & Atmosphere", "Duration for sky texture blending"},
        {"fWeatherCloudChangeTime", "Clouds", "Duration for cloud layer transition"},
        {"fWeatherWindChangeTime", "Atmosphere", "Wind velocity transition smoothing time"},
        {"fWeatherPrecipitationFadeTime", "Precipitation", "Rain and snow fade in/out duration"},
        {"fSunGlareTransitionTime", "Sun & Glare", "Sun glare fade transition time"},
        {"fSunBaseSize", "Sun & Glare", "Base angular radius of sun disk"},
        {"fSunGlareSize", "Sun & Glare", "Sun glare overlay radius"},
        {"fSunShadowRange", "Sun & Glare", "Maximum cast distance for solar shadows"},
        {"fSnowSpecular", "Precipitation", "Specular highlight intensity on snowfall particles"},
        {"fRainSpecular", "Precipitation", "Specular highlight intensity on raindrops"},
        {"fAuroraAlpha", "Sky & Atmosphere", "Aurora layer maximum alpha opacity"},
        {"fFogMax", "Fog", "Maximum atmospheric fog thickness coefficient"},
        {"fFogMin", "Fog", "Minimum atmospheric fog thickness coefficient"},
        {"fNearFogDistance", "Fog", "Distance where atmospheric fog begins"},
        {"fFarFogDistance", "Fog", "Distance where atmospheric fog reaches maximum opacity"},
        {"fCloudSpeed", "Clouds", "Cloud scrolling speed multiplier"},
        {"fCloudAlpha", "Clouds", "Base cloud layer opacity"},
        {"fCloudSunAlpha", "Clouds", "Cloud opacity modifier when occluding sun"}
    };

    static const QList<SettingMeta> morrowindWeather = {
        {"fWeatherTransitionTime", "Transitions", "Seconds for weather transition"},
        {"fWeatherTransitionRadius", "Transitions", "Distance radius for cell weather transitions"}
    };

    if (game == GameFormat::Game::Morrowind)
        return morrowindWeather;
    return skyrimWeather;
}

static const QList<SettingMeta>& lightingCatalog(GameFormat::Game)
{
    static const QList<SettingMeta> skyrimLighting = {
        {"fLightLODRange", "Light LOD", "Base distance threshold for dynamic light LOD"},
        {"fLightLODRangeMin", "Light LOD", "Minimum distance threshold for dynamic light rendering"},
        {"fLightLODRangeMax", "Light LOD", "Maximum distance threshold for dynamic light rendering"},
        {"fLightFadeStart", "Light Falloff", "Fraction of radius where light attenuation begins"},
        {"fLightFadeDist", "Light Falloff", "Light fade-out distance past radius limit"},
        {"fLightColorAmbient", "Ambient", "Default ambient light intensity modifier"},
        {"fLightColorDiffuse", "Diffuse", "Default diffuse light intensity modifier"},
        {"fLightColorSpecular", "Specular", "Default specular reflection modifier"},
        {"fSpecularLODRange", "Specular", "Distance limit for specular light calculations"},
        {"fShadowDirDistance", "Shadows", "Directional sunlight shadow projection distance"},
        {"fShadowFadeDistance", "Shadows", "Shadow edge fade blend distance"},
        {"fShadowBias", "Shadows", "Depth bias to mitigate shadow acne artifacts"},
        {"fShadowRange", "Shadows", "Point/spot light maximum shadow casting range"},
        {"fShadowDistance", "Shadows", "Shadow rendering cutoff distance"},
        {"fShadowFilterRadius", "Shadows", "Soft shadow filter blur radius"},
        {"fDynamicShadowDistance", "Shadows", "Dynamic object shadow draw distance"},
        {"fDynamicShadowFadeDistance", "Shadows", "Dynamic object shadow fade range"},
        {"fAmbientShadowDistance", "Shadows", "Ambient occlusion shadow distance limit"},
        {"fLightRadiusMin", "Light Falloff", "Minimum allowed light radius"},
        {"fLightRadiusMax", "Light Falloff", "Maximum allowed light radius"},
        {"iMaxLights", "Limits", "Maximum concurrent dynamic lights per scene"},
        {"iMaxShadows", "Limits", "Maximum concurrent shadow-casting lights"},
        {"iMaxShadowFilter", "Limits", "Maximum shadow filtering sample quality level"},
        {"iMaxLightRadius", "Limits", "Maximum clamp for dynamic light radius"},
        {"bDynamicShadows", "Shadows", "Toggle for dynamic shadow casting"},
        {"bShadowsOnGrass", "Shadows", "Toggle for casting shadows onto grass geometry"},
        {"bActorShadows", "Shadows", "Toggle for character and creature shadow casting"}
    };

    return skyrimLighting;
}

static bool isCatalogWeather(const QString& name, GameFormat::Game game)
{
    for (const auto& meta : weatherCatalog(game)) {
        if (meta.name.compare(name, Qt::CaseInsensitive) == 0)
            return true;
    }
    return name.startsWith("fWeather", Qt::CaseInsensitive) ||
           name.startsWith("fSun", Qt::CaseInsensitive) ||
           name.startsWith("fFog", Qt::CaseInsensitive) ||
           name.startsWith("fCloud", Qt::CaseInsensitive) ||
           name.startsWith("fSky", Qt::CaseInsensitive) ||
           name.startsWith("fRain", Qt::CaseInsensitive) ||
           name.startsWith("fSnow", Qt::CaseInsensitive) ||
           name.startsWith("fAurora", Qt::CaseInsensitive);
}

static bool isCatalogLighting(const QString& name, GameFormat::Game game)
{
    for (const auto& meta : lightingCatalog(game)) {
        if (meta.name.compare(name, Qt::CaseInsensitive) == 0)
            return true;
    }
    return name.startsWith("fLight", Qt::CaseInsensitive) ||
           name.startsWith("fShadow", Qt::CaseInsensitive) ||
           name.startsWith("iMaxLight", Qt::CaseInsensitive) ||
           name.startsWith("iMaxShadow", Qt::CaseInsensitive) ||
           name.startsWith("bDynamicShadow", Qt::CaseInsensitive) ||
           name.startsWith("bShadow", Qt::CaseInsensitive) ||
           name.startsWith("fAmbient", Qt::CaseInsensitive) ||
           name.startsWith("fSpecular", Qt::CaseInsensitive);
}

static QString getSettingDescription(const QString& name, GameFormat::Game game, bool isWeather)
{
    const auto& catalog = isWeather ? weatherCatalog(game) : lightingCatalog(game);
    for (const auto& meta : catalog) {
        if (meta.name.compare(name, Qt::CaseInsensitive) == 0)
            return QString("[%1] %2").arg(meta.category, meta.description);
    }
    return isWeather ? QStringLiteral("Game setting controlling weather/atmospheric simulation.")
                     : QStringLiteral("Game setting controlling dynamic lighting/shadows.");
}

} // namespace WeatherLightCatalog

WeatherLightEditor::WeatherLightEditor(Data* data, std::function<bool()> saveCallback,
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
      mSelectedValue()
{
    LOG_INFO("WeatherLightEditor created");
    setupUI();
    loadSettings();
}

WeatherLightEditor::~WeatherLightEditor()
{
}

void WeatherLightEditor::setupUI()
{
    setWindowTitle("Lighting & Weather Editor");
    setMinimumSize(1200, 800);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    auto* topBar = new QHBoxLayout();
    QLineEdit* searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Search settings...");
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

    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    auto* detailsGroup = new QGroupBox("GMST Details");
    auto* detailsLayout = new QVBoxLayout(detailsGroup);
    mDetailEdit = new QTextEdit();
    mDetailEdit->setReadOnly(true);
    mDetailEdit->setFontPointSize(10);
    mDetailEdit->setMaximumHeight(180);
    detailsLayout->addWidget(mDetailEdit);
    rightLayout->addWidget(detailsGroup);
    rightLayout->addStretch(1);

    splitter->addWidget(rightPanel);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    mainLayout->addWidget(splitter, 1);

    auto* buttonBar = new QHBoxLayout();
    mAddSettingButton = new QPushButton("Add Setting");
    buttonBar->addWidget(mAddSettingButton);

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

    connect(mTree, &QTreeWidget::itemClicked, this, &WeatherLightEditor::onNodeSelected);
    connect(mAddSettingButton, &QPushButton::clicked, this, &WeatherLightEditor::onAddSetting);
    connect(mEditButton, &QPushButton::clicked, this, &WeatherLightEditor::onEditSetting);
    connect(mDeleteButton, &QPushButton::clicked, this, &WeatherLightEditor::onDeleteSetting);
    connect(mSaveButton, &QPushButton::clicked, this, &WeatherLightEditor::onSave);
}

void WeatherLightEditor::loadSettings()
{
    mTree->clear();
    mSelectedName.clear();
    mSelectedValue.clear();
    mSelectedType.clear();
    mSelectedFormId = 0;

    GameFormat::Game game = mData ? mData->currentGame() : GameFormat::Game::Unknown;

    // 1. Weather Records (WTHR)
    auto& wthrCollection = mData->getWthrCollection();
    auto wthrRecords = wthrCollection.getRecords();
    QTreeWidgetItem* weatherRecGroup = new QTreeWidgetItem(mTree);
    weatherRecGroup->setText(0, "Weather Records (WTHR)");
    weatherRecGroup->setText(1, "GROUP");
    weatherRecGroup->setText(2, QString("%1 weather records").arg(wthrRecords.size()));

    for (const auto& record : wthrRecords) {
        if (record.state == State_Erased) continue;
        const WthrRecord& wthr = record.get();
        QTreeWidgetItem* item = new QTreeWidgetItem(weatherRecGroup);
        item->setText(0, wthr.editorId.isEmpty() ? QString("Weather_%1").arg(wthr.formId, 8, 16, QChar('0')).toUpper() : wthr.editorId);
        item->setText(1, "WTHR");
        item->setText(2, wthr.sunTexture.isEmpty() ? QString("(default sun)") : wthr.sunTexture);
        item->setData(0, Qt::UserRole, QStringLiteral("WTHR"));
        item->setData(0, Qt::UserRole + 1, wthr.editorId);
        item->setData(0, Qt::UserRole + 2, wthr.formId);
    }

    // 2. Light Records (LIGH)
    auto& lighCollection = mData->getLighCollection();
    auto lighRecords = lighCollection.getRecords();
    QTreeWidgetItem* lightRecGroup = new QTreeWidgetItem(mTree);
    lightRecGroup->setText(0, "Light Records (LIGH)");
    lightRecGroup->setText(1, "GROUP");
    lightRecGroup->setText(2, QString("%1 light records").arg(lighRecords.size()));

    for (const auto& record : lighRecords) {
        if (record.state == State_Erased) continue;
        const LighRecord& ligh = record.get();
        QTreeWidgetItem* item = new QTreeWidgetItem(lightRecGroup);
        item->setText(0, ligh.editorId.isEmpty() ? QString("Light_%1").arg(ligh.formId, 8, 16, QChar('0')).toUpper() : ligh.editorId);
        item->setText(1, "LIGH");
        item->setText(2, QString("Radius: %1, Color: 0x%2").arg(ligh.radius).arg(ligh.color, 6, 16, QChar('0')).toUpper());
        item->setData(0, Qt::UserRole, QStringLiteral("LIGH"));
        item->setData(0, Qt::UserRole + 1, ligh.editorId);
        item->setData(0, Qt::UserRole + 2, ligh.formId);
    }

    // 3. Game Settings (GMST) - Weather
    auto& gmstCollection = mData->getGameSettings();
    auto gmstRecords = gmstCollection.getRecords();

    QTreeWidgetItem* weatherGroup = new QTreeWidgetItem(mTree);
    weatherGroup->setText(0, "Weather Game Settings (GMST)");
    weatherGroup->setText(1, "GROUP");

    QTreeWidgetItem* lightingGroup = new QTreeWidgetItem(mTree);
    lightingGroup->setText(0, "Lighting Game Settings (GMST)");
    lightingGroup->setText(1, "GROUP");

    int weatherCount = 0;
    int lightingCount = 0;

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

        bool isWeather = WeatherLightCatalog::isCatalogWeather(name, game);
        bool isLighting = WeatherLightCatalog::isCatalogLighting(name, game);

        if (isWeather) {
            QTreeWidgetItem* item = new QTreeWidgetItem(weatherGroup);
            item->setText(0, name);
            item->setText(1, "GMST");
            item->setText(2, value);
            item->setData(0, Qt::UserRole, QStringLiteral("GMST_WEATHER"));
            item->setData(0, Qt::UserRole + 1, name);
            item->setData(0, Qt::UserRole + 2, value);
            weatherCount++;
        } else if (isLighting) {
            QTreeWidgetItem* item = new QTreeWidgetItem(lightingGroup);
            item->setText(0, name);
            item->setText(1, "GMST");
            item->setText(2, value);
            item->setData(0, Qt::UserRole, QStringLiteral("GMST_LIGHTING"));
            item->setData(0, Qt::UserRole + 1, name);
            item->setData(0, Qt::UserRole + 2, value);
            lightingCount++;
        }
    }

    weatherGroup->setText(2, QString("Weather Settings (%1 settings)").arg(weatherCount));
    lightingGroup->setText(2, QString("Lighting Settings (%1 settings)").arg(lightingCount));

    mTree->expandAll();
    mStatusLabel->setText(QString("Loaded %1 WTHR, %2 LIGH, %3 weather settings, %4 lighting settings")
        .arg(wthrRecords.size()).arg(lighRecords.size()).arg(weatherCount).arg(lightingCount));
    LOG_INFO(QString("Loaded %1 WTHR, %2 LIGH, %3 weather GMST, %4 lighting GMST")
        .arg(wthrRecords.size()).arg(lighRecords.size()).arg(weatherCount).arg(lightingCount));
}
void WeatherLightEditor::refreshTree()
{
    loadSettings();
}

void WeatherLightEditor::onNodeSelected(QTreeWidgetItem* item, int column)
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

    if (type == QLatin1String("WTHR"))
    {
        mSelectedFormId = item->data(0, Qt::UserRole + 2).toUInt();
        auto& coll = mData->getWthrCollection();
        int idx = coll.searchId(mSelectedName);
        if (idx >= 0)
        {
            const auto& wthr = coll.getRecord(idx).get();
            QString text;
            text += QString("<h2>Weather: %1</h2>").arg(wthr.editorId);
            text += QString("<p><b>FormID:</b> 0x%1</p>").arg(wthr.formId, 8, 16, QChar('0')).toUpper();
            text += QString("<p><b>Sun Texture:</b> %1</p>").arg(wthr.sunTexture.isEmpty() ? "(default)" : wthr.sunTexture);
            text += QString("<p><b>Flags:</b> 0x%1</p>").arg(wthr.flags, 8, 16, QChar('0')).toUpper();
            text += "<hr><p><b>Type:</b> Weather Record (WTHR)</p>";
            mDetailEdit->setHtml(text);
        }
    }
    else if (type == QLatin1String("LIGH"))
    {
        mSelectedFormId = item->data(0, Qt::UserRole + 2).toUInt();
        auto& coll = mData->getLighCollection();
        int idx = coll.searchId(mSelectedName);
        if (idx >= 0)
        {
            const auto& ligh = coll.getRecord(idx).get();
            QString text;
            text += QString("<h2>Light: %1</h2>").arg(ligh.editorId);
            text += QString("<p><b>FormID:</b> 0x%1</p>").arg(ligh.formId, 8, 16, QChar('0')).toUpper();
            text += QString("<p><b>Full Name:</b> %1</p>").arg(ligh.fullName);
            text += QString("<p><b>Radius:</b> %1</p>").arg(ligh.radius);
            text += QString("<p><b>Color:</b> 0x%1</p>").arg(ligh.color, 6, 16, QChar('0')).toUpper();
            text += QString("<p><b>Falloff:</b> %1</p>").arg(ligh.falloff);
            text += QString("<p><b>FOV:</b> %1</p>").arg(ligh.fov);
            if (!ligh.modelPath.isEmpty())
                text += QString("<p><b>Model:</b> %1</p>").arg(ligh.modelPath);
            text += "<hr><p><b>Type:</b> Light Record (LIGH)</p>";
            mDetailEdit->setHtml(text);
        }
    }
    else // GMST
    {
        mSelectedValue = item->data(0, Qt::UserRole + 2).toString();
        mSelectedFormId = 0;
        showSettingDetails(mSelectedName, mSelectedValue);
    }
}

void WeatherLightEditor::showSettingDetails(const QString& name, const QString& value)
{
    GameFormat::Game game = mData ? mData->currentGame() : GameFormat::Game::Unknown;
    bool isWeather = mSelectedType == QLatin1String("GMST_WEATHER");
    QString desc = WeatherLightCatalog::getSettingDescription(name, game, isWeather);

    QString text;
    text += QString("<h2>%1</h2>").arg(name);
    text += QString("<p><b>Current Value:</b> %1</p>").arg(value);
    text += "<hr>";
    text += "<p><b>Description:</b></p>";
    text += QString("<p>%1</p>").arg(desc);
    text += "<p><b>Type:</b> GameSetting (GMST)</p>";

    mDetailEdit->setHtml(text);
}

void WeatherLightEditor::onAddSetting()
{
    QStringList options;
    options << tr("Weather Record (WTHR)")
            << tr("Light Record (LIGH)")
            << tr("Weather Game Setting (GMST)")
            << tr("Lighting Game Setting (GMST)");

    bool ok = false;
    QString choice = QInputDialog::getItem(this, tr("Add Item"),
        tr("Select item type to create:"), options, 0, false, &ok);
    if (!ok || choice.isEmpty()) return;

    if (choice.contains("WTHR"))
    {
        QString id = QInputDialog::getText(this, tr("Add Weather Record"),
            tr("Enter Editor ID for new Weather:"), QLineEdit::Normal, "", &ok);
        if (!ok || id.trimmed().isEmpty()) return;
        const QString finalId = id.trimmed();

        auto& coll = mData->getWthrCollection();
        if (coll.searchId(finalId) >= 0)
        {
            QMessageBox::warning(this, tr("Add Weather Record"),
                tr("A weather record named '%1' already exists.").arg(finalId));
            return;
        }

        WthrRecord rec;
        rec.blank();
        rec.initComponents();
        rec.editorId = finalId;
        try {
            rec.formId = mData->createNewRecord(CkId::Type_Wthr_, finalId);
        } catch (const std::exception& e) {
            QMessageBox::warning(this, tr("Add Weather Record"),
                tr("Could not allocate a FormID: %1").arg(QString::fromUtf8(e.what())));
            return;
        }

        auto* table = qobject_cast<IdTable*>(mData->getTableModel(CkId::Type_Wthr_));
        const int appendIdx = coll.getAppendIndex(finalId, CkId::Type_Wthr_);
        Record<WthrRecord> record(State_ModifiedOnly, nullptr, &rec);
        if (mData->getUndoStack() && table)
        {
            mData->getUndoStack()->push(new AddRecordCommand(
                table, &coll, appendIdx, record,
                QStringLiteral("Add Weather Record: %1").arg(finalId)));
        }
        else
        {
            coll.appendRecord(record, CkId::Type_Wthr_);
        }
        LOG_INFO(QString("Added weather record '%1'").arg(finalId));
        mStatusLabel->setText(QString("Added weather '%1'").arg(finalId));
        refreshTree();
    }
    else if (choice.contains("LIGH"))
    {
        QString id = QInputDialog::getText(this, tr("Add Light Record"),
            tr("Enter Editor ID for new Light:"), QLineEdit::Normal, "", &ok);
        if (!ok || id.trimmed().isEmpty()) return;
        const QString finalId = id.trimmed();

        auto& coll = mData->getLighCollection();
        if (coll.searchId(finalId) >= 0)
        {
            QMessageBox::warning(this, tr("Add Light Record"),
                tr("A light record named '%1' already exists.").arg(finalId));
            return;
        }

        LighRecord rec;
        rec.blank();
        rec.editorId = finalId;
        rec.radius = 500;
        rec.color = 0x00FFFFFF;
        try {
            rec.formId = mData->createNewRecord(CkId::Type_Ligh_, finalId);
        } catch (const std::exception& e) {
            QMessageBox::warning(this, tr("Add Light Record"),
                tr("Could not allocate a FormID: %1").arg(QString::fromUtf8(e.what())));
            return;
        }

        auto* table = qobject_cast<IdTable*>(mData->getTableModel(CkId::Type_Ligh_));
        const int appendIdx = coll.getAppendIndex(finalId, CkId::Type_Ligh_);
        Record<LighRecord> record(State_ModifiedOnly, nullptr, &rec);
        if (mData->getUndoStack() && table)
        {
            mData->getUndoStack()->push(new AddRecordCommand(
                table, &coll, appendIdx, record,
                QStringLiteral("Add Light Record: %1").arg(finalId)));
        }
        else
        {
            coll.appendRecord(record, CkId::Type_Ligh_);
        }
        LOG_INFO(QString("Added light record '%1'").arg(finalId));
        mStatusLabel->setText(QString("Added light '%1'").arg(finalId));
        refreshTree();
    }
    else
    {
        QString defaultName = choice.contains("Weather") ? "fWeatherCustom" : "fLightCustom";
        QString name = QInputDialog::getText(this, tr("Add Setting"),
            tr("Enter setting name:"), QLineEdit::Normal, defaultName, &ok);
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
        LOG_INFO(QString("Added setting '%1' with value '%2'").arg(finalId, value));
        mStatusLabel->setText(QString("Added setting '%1'").arg(finalId));
        refreshTree();
    }
}

void WeatherLightEditor::onEditSetting()
{
    if (mSelectedName.isEmpty()) return;

    if (mSelectedType == QLatin1String("WTHR"))
    {
        auto& coll = mData->getWthrCollection();
        int idx = coll.searchId(mSelectedName);
        if (idx < 0) return;

        WthrRecord original = coll.getRecord(idx).get();
        bool ok = false;
        QString newTexture = QInputDialog::getText(this, tr("Edit Weather"),
            tr("Enter sun texture path for '%1':").arg(mSelectedName),
            QLineEdit::Normal, original.sunTexture, &ok);
        if (!ok) return;

        WthrRecord edited = original;
        edited.sunTexture = newTexture.trimmed();
        if (mData->getUndoStack())
        {
            auto* cmd = new EditRecordCommand<WthrRecord>(
                &coll, idx, original, edited,
                QStringLiteral("Edit Weather: %1").arg(mSelectedName));
            cmd && !(original == edited) ? mData->getUndoStack()->push(cmd) : delete cmd;
        }
        else
        {
            coll.getRecord(idx).get() = edited;
            coll.getRecord(idx).state = State_Modified;
        }
        LOG_INFO(QString("Updated weather '%1'").arg(mSelectedName));
        refreshTree();
    }
    else if (mSelectedType == QLatin1String("LIGH"))
    {
        auto& coll = mData->getLighCollection();
        int idx = coll.searchId(mSelectedName);
        if (idx < 0) return;

        LighRecord original = coll.getRecord(idx).get();
        bool ok = false;
        int newRadius = QInputDialog::getInt(this, tr("Edit Light"),
            tr("Enter radius for '%1':").arg(mSelectedName),
            static_cast<int>(original.radius), 0, 100000, 10, &ok);
        if (!ok) return;

        LighRecord edited = original;
        edited.radius = static_cast<quint32>(newRadius);
        if (mData->getUndoStack())
        {
            auto* cmd = new EditRecordCommand<LighRecord>(
                &coll, idx, original, edited,
                QStringLiteral("Edit Light: %1").arg(mSelectedName));
            cmd && !(original == edited) ? mData->getUndoStack()->push(cmd) : delete cmd;
        }
        else
        {
            coll.getRecord(idx).get() = edited;
            coll.getRecord(idx).state = State_Modified;
        }
        LOG_INFO(QString("Updated light '%1'").arg(mSelectedName));
        refreshTree();
    }
    else // GMST
    {
        bool ok = false;
        QString newValue = QInputDialog::getText(this, tr("Edit Setting"),
            tr("Enter new value for '%1':").arg(mSelectedName),
            QLineEdit::Normal, mSelectedValue, &ok);
        if (!ok) return;

        auto& coll = mData->getGameSettings();
        int idx = coll.searchId(mSelectedName);
        if (idx < 0) return;

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

void WeatherLightEditor::onDeleteSetting()
{
    if (mSelectedName.isEmpty()) return;

    auto reply = QMessageBox::question(this, tr("Delete Item"),
        tr("Are you sure you want to delete '%1'?\n\nThis action cannot be undone.").arg(mSelectedName),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (mSelectedType == QLatin1String("WTHR"))
        {
            mData->getWthrCollection().removeRecordWithUndo(mSelectedName, mData->getUndoStack());
            LOG_INFO(QString("Deleted weather record '%1'").arg(mSelectedName));
        }
        else if (mSelectedType == QLatin1String("LIGH"))
        {
            mData->getLighCollection().removeRecordWithUndo(mSelectedName, mData->getUndoStack());
            LOG_INFO(QString("Deleted light record '%1'").arg(mSelectedName));
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

void WeatherLightEditor::onSave()
{
    if (!mSaveCallback || !mSaveCallback())
    {
        QMessageBox::warning(this, tr("Save"), tr("The active document could not be saved."));
        return;
    }
    mStatusLabel->setText(tr("Active document saved."));
}
