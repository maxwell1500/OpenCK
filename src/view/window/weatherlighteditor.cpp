#include "weatherlighteditor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/collection.hpp"
#include "../../model/world/collection_impl.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/tools/editrecordcommand.hpp"
#include "../../model/tools/undostack.hpp"
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

} // namespace

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
    mAddSettingButton->setEnabled(false);
    mAddSettingButton->setToolTip("Adding game settings is not yet supported.");
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

    auto& coll = mData->getGameSettings();
    int idx = coll.searchId(mSelectedName);
    if (idx < 0)
    {
        LOG_WARNING(QString("Cannot edit setting '%1': not found in game settings").arg(mSelectedName));
        return;
    }

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

    mSelectedValue = newValue;
    LOG_INFO(QString("Updated setting '%1' to '%2'").arg(mSelectedName).arg(newValue));
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
        if (mData->getGameSettings().removeRecordWithUndo(mSelectedName, mData->getUndoStack()))
        {
            LOG_INFO(QString("Deleted setting '%1'").arg(mSelectedName));
        }
        else
        {
            LOG_WARNING(QString("Could not delete setting '%1': not found in game settings").arg(mSelectedName));
        }
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
