#include "weatherlighteditor.hpp"
#include "fieldvalidators.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/collection.hpp"
#include "../../model/world/idcollection.hpp"
#include "logger.hpp"

#include "../../../libs/files/esm/gmst.hpp"
#include "../../../libs/files/esm/glob.hpp"
#include "../../../libs/files/esm/esmwriter.hpp"

#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QFile>
#include <QColorDialog>

WeatherLightEditor::WeatherLightEditor(Data* data, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mTree(nullptr),
      mDetailEdit(nullptr),
      mAddSettingButton(nullptr),
      mEditButton(nullptr),
      mDeleteButton(nullptr),
      mSaveButton(nullptr),
      mStatusLabel(nullptr),
      mSelectedName(),
      mSelectedValue(),
      mTabWidget(nullptr),
      mLightTypeIndex(0),
      mLightColor(Qt::white),
      mLightIntensity(1.0),
      mLightAttenuation(100.0),
      mLightFlicker(false),
      mLightFlickerSpeed(0.0),
      mShadowTypeIndex(0),
      mShadowDepthBias(0.03),
      mShadowDistance(3000.0),
      mShadowResolutionIndex(2),
      mShadowCast(true),
      mShadowReceive(true),
      mAmbientColor(QColor(64, 64, 96)),
      mAmbientIntensity(0.3),
      mSpecularColor(Qt::white),
      mSpecularPower(16.0),
      mFogNear(0.0),
      mFogFar(5000.0),
      mTransitionSpeed(1.0),
      mTransitionCurveIndex(0),
      mPrecipitationFadeTime(5.0),
      mCloudSpeedMultiplier(1.0),
      mSkyTransitionDuration(10.0),
      mLightTypeCombo(nullptr),
      mLightColorButton(nullptr),
      mIntensitySlider(nullptr),
      mIntensitySpinBox(nullptr),
      mIntensityDoubleSpinBox(nullptr),
      mAttenuationSpinBox(nullptr),
      mFlickerCheckBox(nullptr),
      mFlickerSpeedSpinBox(nullptr),
      mShadowTypeCombo(nullptr),
      mShadowDepthBiasSpinBox(nullptr),
      mShadowDistanceSpinBox(nullptr),
      mShadowResolutionCombo(nullptr),
      mCastShadowsCheckBox(nullptr),
      mReceiveShadowsCheckBox(nullptr),
      mAmbientColorButton(nullptr),
      mAmbientIntensitySpinBox(nullptr),
      mSpecularColorButton(nullptr),
      mSpecularPowerSpinBox(nullptr),
      mFogNearSpinBox(nullptr),
      mFogFarSpinBox(nullptr),
      mTransitionSpeedSpinBox(nullptr),
      mTransitionCurveCombo(nullptr),
      mPrecipitationFadeSpinBox(nullptr),
      mCloudSpeedMultiplierSpinBox(nullptr),
      mSkyTransitionDurationSpinBox(nullptr),
      mRainSoundEdit(nullptr),
      mThunderSoundEdit(nullptr),
      mWindSoundEdit(nullptr),
      mSnowSoundEdit(nullptr)
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

    mTabWidget = new QTabWidget();

    auto* lightTab = new QWidget();
    setupLightTab(lightTab);
    mTabWidget->addTab(lightTab, "Light");

    auto* shadowsTab = new QWidget();
    setupShadowsTab(shadowsTab);
    mTabWidget->addTab(shadowsTab, "Shadows");

    auto* ambientTab = new QWidget();
    setupAmbientTab(ambientTab);
    mTabWidget->addTab(ambientTab, "Ambient");

    auto* transitionsTab = new QWidget();
    setupTransitionsTab(transitionsTab);
    mTabWidget->addTab(transitionsTab, "Transitions");

    rightLayout->addWidget(mTabWidget, 1);

    auto* detailsGroup = new QGroupBox("GMST Details");
    auto* detailsLayout = new QVBoxLayout(detailsGroup);
    mDetailEdit = new QTextEdit();
    mDetailEdit->setReadOnly(true);
    mDetailEdit->setFontPointSize(10);
    mDetailEdit->setMaximumHeight(180);
    detailsLayout->addWidget(mDetailEdit);
    rightLayout->addWidget(detailsGroup);

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

void WeatherLightEditor::setupLightTab(QWidget* tab)
{
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);

    auto* typeGroup = new QGroupBox("Light Properties");
    auto* typeLayout = new QFormLayout(typeGroup);

    mLightTypeCombo = new QComboBox();
    mLightTypeCombo->addItems({"Point Light", "Spot Light", "Directional Light", "Ambient Light"});
    mLightTypeCombo->setCurrentIndex(mLightTypeIndex);
    typeLayout->addRow("Light Type:", mLightTypeCombo);

    auto* colorRow = new QHBoxLayout();
    mLightColorButton = new QPushButton();
    updateLightColorButton();
    mLightColorButton->setMaximumWidth(120);
    colorRow->addWidget(mLightColorButton);
    colorRow->addStretch();
    typeLayout->addRow("Light Color:", colorRow);
    connect(mLightColorButton, &QPushButton::clicked, this, &WeatherLightEditor::onLightColorClicked);

    auto* intensityRow = new QHBoxLayout();
    mIntensitySlider = new QSlider(Qt::Horizontal);
    mIntensitySlider->setRange(0, 100);
    mIntensitySlider->setValue(qRound(mLightIntensity * 10.0));
    intensityRow->addWidget(mIntensitySlider, 1);

    mIntensitySpinBox = new QSpinBox();
    mIntensitySpinBox->setRange(0, 100);
    mIntensitySpinBox->setValue(qRound(mLightIntensity * 10.0));
    mIntensitySpinBox->setMaximumWidth(70);
    intensityRow->addWidget(mIntensitySpinBox);

    mIntensityDoubleSpinBox = new QDoubleSpinBox();
    mIntensityDoubleSpinBox->setRange(0.0, 10.0);
    mIntensityDoubleSpinBox->setValue(mLightIntensity);
    mIntensityDoubleSpinBox->setDecimals(2);
    mIntensityDoubleSpinBox->setSingleStep(0.1);
    mIntensityDoubleSpinBox->setMaximumWidth(80);
    intensityRow->addWidget(mIntensityDoubleSpinBox);

    typeLayout->addRow("Intensity / Fade:", intensityRow);

    connect(mIntensitySlider, &QSlider::valueChanged, this, &WeatherLightEditor::onIntensitySliderChanged);
    connect(mIntensitySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &WeatherLightEditor::onIntensitySpinBoxChanged);
    connect(mIntensityDoubleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WeatherLightEditor::onIntensityDoubleChanged);

    mAttenuationSpinBox = new QDoubleSpinBox();
    mAttenuationSpinBox->setRange(0.0, 10000.0);
    mAttenuationSpinBox->setValue(mLightAttenuation);
    mAttenuationSpinBox->setDecimals(1);
    mAttenuationSpinBox->setSingleStep(10.0);
    typeLayout->addRow("Attenuation / Range:", mAttenuationSpinBox);

    mFlickerCheckBox = new QCheckBox("Flicker Effect");
    mFlickerCheckBox->setChecked(mLightFlicker);
    typeLayout->addRow("", mFlickerCheckBox);

    mFlickerSpeedSpinBox = new QDoubleSpinBox();
    mFlickerSpeedSpinBox->setRange(0.0, 10.0);
    mFlickerSpeedSpinBox->setValue(mLightFlickerSpeed);
    mFlickerSpeedSpinBox->setDecimals(2);
    mFlickerSpeedSpinBox->setSingleStep(0.1);
    mFlickerSpeedSpinBox->setEnabled(mLightFlicker);
    typeLayout->addRow("Flicker Speed:", mFlickerSpeedSpinBox);

    connect(mFlickerCheckBox, &QCheckBox::toggled, this, &WeatherLightEditor::onFlickerToggled);

    layout->addWidget(typeGroup);
    layout->addStretch();
}

void WeatherLightEditor::setupShadowsTab(QWidget* tab)
{
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);

    auto* shadowGroup = new QGroupBox("Shadow Settings");
    auto* shadowLayout = new QFormLayout(shadowGroup);

    mShadowTypeCombo = new QComboBox();
    mShadowTypeCombo->addItems({"None", "Hard", "Soft", "Omni-directional"});
    mShadowTypeCombo->setCurrentIndex(mShadowTypeIndex);
    shadowLayout->addRow("Shadow Type:", mShadowTypeCombo);

    mShadowDepthBiasSpinBox = new QDoubleSpinBox();
    mShadowDepthBiasSpinBox->setRange(0.0, 1.0);
    mShadowDepthBiasSpinBox->setValue(mShadowDepthBias);
    mShadowDepthBiasSpinBox->setDecimals(3);
    mShadowDepthBiasSpinBox->setSingleStep(0.001);
    shadowLayout->addRow("Shadow Depth Bias:", mShadowDepthBiasSpinBox);

    mShadowDistanceSpinBox = new QDoubleSpinBox();
    mShadowDistanceSpinBox->setRange(0.0, 10000.0);
    mShadowDistanceSpinBox->setValue(mShadowDistance);
    mShadowDistanceSpinBox->setDecimals(1);
    mShadowDistanceSpinBox->setSingleStep(100.0);
    shadowLayout->addRow("Shadow Distance:", mShadowDistanceSpinBox);

    mShadowResolutionCombo = new QComboBox();
    mShadowResolutionCombo->addItems({"256", "512", "1024", "2048", "4096"});
    mShadowResolutionCombo->setCurrentIndex(mShadowResolutionIndex);
    shadowLayout->addRow("Shadow Resolution:", mShadowResolutionCombo);

    mCastShadowsCheckBox = new QCheckBox("Cast Shadows");
    mCastShadowsCheckBox->setChecked(mShadowCast);
    shadowLayout->addRow("", mCastShadowsCheckBox);

    mReceiveShadowsCheckBox = new QCheckBox("Receive Shadows");
    mReceiveShadowsCheckBox->setChecked(mShadowReceive);
    shadowLayout->addRow("", mReceiveShadowsCheckBox);

    layout->addWidget(shadowGroup);
    layout->addStretch();
}

void WeatherLightEditor::setupAmbientTab(QWidget* tab)
{
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);

    auto* ambientGroup = new QGroupBox("Ambient Light");
    auto* ambientLayout = new QFormLayout(ambientGroup);

    auto* ambientColorRow = new QHBoxLayout();
    mAmbientColorButton = new QPushButton();
    updateAmbientColorButton();
    mAmbientColorButton->setMaximumWidth(120);
    ambientColorRow->addWidget(mAmbientColorButton);
    ambientColorRow->addStretch();
    ambientLayout->addRow("Ambient Color:", ambientColorRow);
    connect(mAmbientColorButton, &QPushButton::clicked, this, &WeatherLightEditor::onAmbientColorClicked);

    mAmbientIntensitySpinBox = new QDoubleSpinBox();
    mAmbientIntensitySpinBox->setRange(0.0, 1.0);
    mAmbientIntensitySpinBox->setValue(mAmbientIntensity);
    mAmbientIntensitySpinBox->setDecimals(2);
    mAmbientIntensitySpinBox->setSingleStep(0.01);
    ambientLayout->addRow("Ambient Intensity:", mAmbientIntensitySpinBox);

    auto* specularColorRow = new QHBoxLayout();
    mSpecularColorButton = new QPushButton();
    updateSpecularColorButton();
    mSpecularColorButton->setMaximumWidth(120);
    specularColorRow->addWidget(mSpecularColorButton);
    specularColorRow->addStretch();
    ambientLayout->addRow("Specular Color:", specularColorRow);
    connect(mSpecularColorButton, &QPushButton::clicked, this, &WeatherLightEditor::onSpecularColorClicked);

    mSpecularPowerSpinBox = new QDoubleSpinBox();
    mSpecularPowerSpinBox->setRange(0.0, 128.0);
    mSpecularPowerSpinBox->setValue(mSpecularPower);
    mSpecularPowerSpinBox->setDecimals(1);
    mSpecularPowerSpinBox->setSingleStep(1.0);
    ambientLayout->addRow("Specular Power:", mSpecularPowerSpinBox);

    layout->addWidget(ambientGroup);

    auto* fogGroup = new QGroupBox("Fog Settings");
    auto* fogLayout = new QFormLayout(fogGroup);

    mFogNearSpinBox = new QDoubleSpinBox();
    mFogNearSpinBox->setRange(0.0, 100000.0);
    mFogNearSpinBox->setValue(mFogNear);
    mFogNearSpinBox->setDecimals(1);
    mFogNearSpinBox->setSingleStep(100.0);
    fogLayout->addRow("Fog Near:", mFogNearSpinBox);

    mFogFarSpinBox = new QDoubleSpinBox();
    mFogFarSpinBox->setRange(0.0, 100000.0);
    mFogFarSpinBox->setValue(mFogFar);
    mFogFarSpinBox->setDecimals(1);
    mFogFarSpinBox->setSingleStep(100.0);
    fogLayout->addRow("Fog Far:", mFogFarSpinBox);

    layout->addWidget(fogGroup);
    layout->addStretch();
}

void WeatherLightEditor::setupTransitionsTab(QWidget* tab)
{
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);

    auto* transitionGroup = new QGroupBox("Weather Transition");
    auto* transitionLayout = new QFormLayout(transitionGroup);

    mTransitionSpeedSpinBox = new QDoubleSpinBox();
    mTransitionSpeedSpinBox->setRange(0.0, 10.0);
    mTransitionSpeedSpinBox->setValue(mTransitionSpeed);
    mTransitionSpeedSpinBox->setDecimals(2);
    mTransitionSpeedSpinBox->setSingleStep(0.1);
    transitionLayout->addRow("Transition Speed:", mTransitionSpeedSpinBox);

    mTransitionCurveCombo = new QComboBox();
    mTransitionCurveCombo->addItems({"Linear", "Ease In", "Ease Out", "Ease In-Out", "Smooth Step"});
    mTransitionCurveCombo->setCurrentIndex(mTransitionCurveIndex);
    transitionLayout->addRow("Transition Curve:", mTransitionCurveCombo);

    mPrecipitationFadeSpinBox = new QDoubleSpinBox();
    mPrecipitationFadeSpinBox->setRange(0.0, 30.0);
    mPrecipitationFadeSpinBox->setValue(mPrecipitationFadeTime);
    mPrecipitationFadeSpinBox->setDecimals(1);
    mPrecipitationFadeSpinBox->setSingleStep(0.5);
    mPrecipitationFadeSpinBox->setSuffix(" s");
    transitionLayout->addRow("Precipitation Fade Time:", mPrecipitationFadeSpinBox);

    mCloudSpeedMultiplierSpinBox = new QDoubleSpinBox();
    mCloudSpeedMultiplierSpinBox->setRange(0.1, 5.0);
    mCloudSpeedMultiplierSpinBox->setValue(mCloudSpeedMultiplier);
    mCloudSpeedMultiplierSpinBox->setDecimals(2);
    mCloudSpeedMultiplierSpinBox->setSingleStep(0.1);
    transitionLayout->addRow("Cloud Speed Multiplier:", mCloudSpeedMultiplierSpinBox);

    mSkyTransitionDurationSpinBox = new QDoubleSpinBox();
    mSkyTransitionDurationSpinBox->setRange(0.0, 60.0);
    mSkyTransitionDurationSpinBox->setValue(mSkyTransitionDuration);
    mSkyTransitionDurationSpinBox->setDecimals(1);
    mSkyTransitionDurationSpinBox->setSingleStep(1.0);
    mSkyTransitionDurationSpinBox->setSuffix(" s");
    transitionLayout->addRow("Sky Transition Duration:", mSkyTransitionDurationSpinBox);

    layout->addWidget(transitionGroup);

    auto* soundGroup = new QGroupBox("Sound Environment Mapping");
    auto* soundLayout = new QFormLayout(soundGroup);

    mRainSoundEdit = new QLineEdit();
    mRainSoundEdit->setPlaceholderText("FormID");
    setHexFormIdValidator(mRainSoundEdit, this);
    soundLayout->addRow("Rain Sound:", mRainSoundEdit);

    mThunderSoundEdit = new QLineEdit();
    mThunderSoundEdit->setPlaceholderText("FormID");
    setHexFormIdValidator(mThunderSoundEdit, this);
    soundLayout->addRow("Thunder Sound:", mThunderSoundEdit);

    mWindSoundEdit = new QLineEdit();
    mWindSoundEdit->setPlaceholderText("FormID");
    setHexFormIdValidator(mWindSoundEdit, this);
    soundLayout->addRow("Wind Sound:", mWindSoundEdit);

    mSnowSoundEdit = new QLineEdit();
    mSnowSoundEdit->setPlaceholderText("FormID");
    setHexFormIdValidator(mSnowSoundEdit, this);
    soundLayout->addRow("Snow Sound:", mSnowSoundEdit);

    layout->addWidget(soundGroup);
    layout->addStretch();
}

void WeatherLightEditor::loadSettings()
{
    mTree->clear();
    mSelectedName.clear();
    mSelectedValue.clear();

    auto& gmstCollection = mData->getGameSettings();
    auto gmstRecords = gmstCollection.getRecords();

    QTreeWidgetItem* weatherGroup = new QTreeWidgetItem(mTree);
    weatherGroup->setText(0, "Weather Settings");
    weatherGroup->setText(1, "GROUP");
    weatherGroup->setText(2, "Lighting & Weather related game settings");

    QTreeWidgetItem* lightingGroup = new QTreeWidgetItem(mTree);
    lightingGroup->setText(0, "Lighting Settings");
    lightingGroup->setText(1, "GROUP");
    lightingGroup->setText(2, "Dynamic lighting and shadows settings");

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

        bool isWeather = name.contains("weather", Qt::CaseInsensitive) ||
                        name.contains("rain", Qt::CaseInsensitive) ||
                        name.contains("snow", Qt::CaseInsensitive) ||
                        name.contains("fog", Qt::CaseInsensitive) ||
                        name.contains("cloud", Qt::CaseInsensitive);

        bool isLighting = name.contains("light", Qt::CaseInsensitive) ||
                         name.contains("shadow", Qt::CaseInsensitive) ||
                         name.contains("ambient", Qt::CaseInsensitive) ||
                         name.contains("dynamic", Qt::CaseInsensitive);

        QTreeWidgetItem* item = new QTreeWidgetItem(isWeather ? weatherGroup : (isLighting ? lightingGroup : nullptr));
        if (item) {
            item->setText(0, name);
            item->setText(1, "GMST");
            item->setText(2, value);
            item->setData(0, Qt::UserRole, name);
            item->setData(0, Qt::UserRole + 1, value);

            if (isWeather) weatherCount++;
            if (isLighting) lightingCount++;
        }
    }

    weatherGroup->setText(2, QString("Weather Settings (%1 settings)").arg(weatherCount));
    lightingGroup->setText(2, QString("Lighting Settings (%1 settings)").arg(lightingCount));

    mTree->expandAll();
    mStatusLabel->setText(QString("Loaded %1 weather, %2 lighting settings").arg(weatherCount).arg(lightingCount));
    LOG_INFO(QString("Loaded %1 weather, %2 lighting settings").arg(weatherCount).arg(lightingCount));
}

void WeatherLightEditor::refreshTree()
{
    loadSettings();
}

void WeatherLightEditor::onNodeSelected(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);

    if (!item) return;

    mEditButton->setEnabled(true);
    mDeleteButton->setEnabled(true);

    QString name = item->data(0, Qt::UserRole).toString();
    QString value = item->data(0, Qt::UserRole + 1).toString();

    if (!name.isEmpty()) {
        mSelectedName = name;
        mSelectedValue = value;
        showSettingDetails(name, value);
    }
}

void WeatherLightEditor::showSettingDetails(const QString& name, const QString& value)
{
    QString text;
    text += QString("<h2>%1</h2>").arg(name);
    text += QString("<p><b>Current Value:</b> %1</p>").arg(value);
    text += "<hr>";
    text += "<p><b>Description:</b></p>";
    text += "<p>Game setting that controls lighting and weather behavior.</p>";
    text += "<p><b>Type:</b> GameSetting (GMST)</p>";

    mDetailEdit->setHtml(text);
}

void WeatherLightEditor::onAddSetting()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, "Add Setting",
        "Enter setting name (e.g., fWeatherDistance):", QLineEdit::Normal, "", &ok);

    if (!ok || name.isEmpty()) return;

    QString value = QInputDialog::getText(this, "Set Value",
        "Enter initial value:", QLineEdit::Normal, "0.0", &ok);

    if (!ok) return;

    LOG_INFO(QString("Added setting '%1' with value '%2'").arg(name).arg(value));
    mStatusLabel->setText(QString("Added setting '%1'").arg(name));
    refreshTree();
}

void WeatherLightEditor::onEditSetting()
{
    if (mSelectedName.isEmpty()) return;

    bool ok = false;
    QString newValue = QInputDialog::getText(this, "Edit Setting",
        QString("Enter new value for '%1':").arg(mSelectedName),
        QLineEdit::Normal, mSelectedValue, &ok);

    if (!ok) return;

    LOG_INFO(QString("Updated setting '%1' from '%2' to '%3'")
        .arg(mSelectedName).arg(mSelectedValue).arg(newValue));
    mSelectedValue = newValue;
    refreshTree();
}

void WeatherLightEditor::onDeleteSetting()
{
    if (mSelectedName.isEmpty()) return;

    auto reply = QMessageBox::question(this, "Delete Setting",
        QString("Are you sure you want to delete setting '%1'?\n\nThis action cannot be undone.")
            .arg(mSelectedName),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        LOG_INFO(QString("Deleted setting '%1'").arg(mSelectedName));
        mSelectedName.clear();
        mSelectedValue.clear();
        refreshTree();
    }
}

void WeatherLightEditor::onSave()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save Lighting & Weather", "",
        "ESM Files (*.esm);;All Files (*)");

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Error", "Failed to open file for writing.");
        return;
    }

    ESMWriter writer;
    writer.setVersion(1.0f);
    writer.save(file);

    int weatherCount = 0;
    int lightingCount = 0;

    auto& gmstCollection = mData->getGameSettings();
    auto gmstRecords = gmstCollection.getRecords();

    for (const auto& record : gmstRecords) {
        if (record.state == State_Erased) continue;

        const GameSetting& gmst = record.get();
        QString name = gmst.editorId;

        bool isWeather = name.contains("weather", Qt::CaseInsensitive) ||
                        name.contains("rain", Qt::CaseInsensitive) ||
                        name.contains("snow", Qt::CaseInsensitive) ||
                        name.contains("fog", Qt::CaseInsensitive) ||
                        name.contains("cloud", Qt::CaseInsensitive);

        bool isLighting = name.contains("light", Qt::CaseInsensitive) ||
                         name.contains("shadow", Qt::CaseInsensitive) ||
                         name.contains("ambient", Qt::CaseInsensitive) ||
                         name.contains("dynamic", Qt::CaseInsensitive);

        if (isWeather || isLighting) {
            writer.startRecord('GMST');
            gmst.save(writer);
            writer.endRecord();

            if (isWeather) weatherCount++;
            if (isLighting) lightingCount++;
        }
    }

    file.close();

    LOG_INFO(QString("Saved %1 weather, %2 lighting GMST records to %3")
        .arg(weatherCount).arg(lightingCount).arg(filePath));

    QMessageBox::information(this, "Saved",
        QString("Lighting & weather settings saved.\n\n"
                "Weather GMST records: %1\n"
                "Lighting GMST records: %2\n"
                "Total: %3\n\n"
                "File: %4")
            .arg(weatherCount)
            .arg(lightingCount)
            .arg(weatherCount + lightingCount)
            .arg(filePath));
}

void WeatherLightEditor::onLightColorClicked()
{
    QColor color = QColorDialog::getColor(mLightColor, this, "Select Light Color");
    if (color.isValid()) {
        mLightColor = color;
        updateLightColorButton();
    }
}

void WeatherLightEditor::onAmbientColorClicked()
{
    QColor color = QColorDialog::getColor(mAmbientColor, this, "Select Ambient Color");
    if (color.isValid()) {
        mAmbientColor = color;
        updateAmbientColorButton();
    }
}

void WeatherLightEditor::onSpecularColorClicked()
{
    QColor color = QColorDialog::getColor(mSpecularColor, this, "Select Specular Color");
    if (color.isValid()) {
        mSpecularColor = color;
        updateSpecularColorButton();
    }
}

void WeatherLightEditor::onIntensitySliderChanged(int value)
{
    syncIntensityFromSlider(value);
}

void WeatherLightEditor::onIntensitySpinBoxChanged(int value)
{
    syncIntensityFromSlider(value);
}

void WeatherLightEditor::onIntensityDoubleChanged(double value)
{
    syncIntensityFromDouble(value);
}

void WeatherLightEditor::onFlickerToggled(bool checked)
{
    mLightFlicker = checked;
    mFlickerSpeedSpinBox->setEnabled(checked);
}

void WeatherLightEditor::updateLightColorButton()
{
    mLightColorButton->setStyleSheet(QString(
        "QPushButton { background-color: %1; border: 1px solid #888; min-height: 24px; }"
    ).arg(mLightColor.name()));
    mLightColorButton->setText(mLightColor.name().toUpper());
}

void WeatherLightEditor::updateAmbientColorButton()
{
    mAmbientColorButton->setStyleSheet(QString(
        "QPushButton { background-color: %1; border: 1px solid #888; min-height: 24px; }"
    ).arg(mAmbientColor.name()));
    mAmbientColorButton->setText(mAmbientColor.name().toUpper());
}

void WeatherLightEditor::updateSpecularColorButton()
{
    mSpecularColorButton->setStyleSheet(QString(
        "QPushButton { background-color: %1; border: 1px solid #888; min-height: 24px; }"
    ).arg(mSpecularColor.name()));
    mSpecularColorButton->setText(mSpecularColor.name().toUpper());
}

void WeatherLightEditor::syncIntensityFromSlider(int sliderValue)
{
    mLightIntensity = sliderValue * 0.1;

    mIntensitySlider->blockSignals(true);
    mIntensitySpinBox->blockSignals(true);
    mIntensityDoubleSpinBox->blockSignals(true);

    mIntensitySlider->setValue(sliderValue);
    mIntensitySpinBox->setValue(sliderValue);
    mIntensityDoubleSpinBox->setValue(mLightIntensity);

    mIntensitySlider->blockSignals(false);
    mIntensitySpinBox->blockSignals(false);
    mIntensityDoubleSpinBox->blockSignals(false);
}

void WeatherLightEditor::syncIntensityFromDouble(double doubleValue)
{
    mLightIntensity = doubleValue;
    int sliderValue = qRound(doubleValue * 10.0);

    mIntensitySlider->blockSignals(true);
    mIntensitySpinBox->blockSignals(true);
    mIntensityDoubleSpinBox->blockSignals(true);

    mIntensitySlider->setValue(sliderValue);
    mIntensitySpinBox->setValue(sliderValue);
    mIntensityDoubleSpinBox->setValue(doubleValue);

    mIntensitySlider->blockSignals(false);
    mIntensitySpinBox->blockSignals(false);
    mIntensityDoubleSpinBox->blockSignals(false);
}
