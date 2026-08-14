#include "watereditor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/collection.hpp"
#include "../../model/world/collection_impl.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/tools/editrecordcommand.hpp"
#include "../../model/tools/undostack.hpp"
#include "logger.hpp"

#include "../../../libs/files/esm/glob.hpp"
#include "../../../libs/files/esm/gmst.hpp"
#include "../../../libs/files/esm/esmwriter.hpp"

#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFile>

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

} // namespace

WaterEditor::WaterEditor(Data* data, QWidget* parent)
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
    mAddSettingButton = new QPushButton("Add Setting");
    mAddSettingButton->setEnabled(false);
    mAddSettingButton->setToolTip("Adding water settings is not yet supported.");
    buttonBar->addWidget(mAddSettingButton);

    mEditButton = new QPushButton("Edit");
    mEditButton->setEnabled(false);
    buttonBar->addWidget(mEditButton);

    mDeleteButton = new QPushButton("Delete");
    mDeleteButton->setEnabled(false);
    buttonBar->addWidget(mDeleteButton);

    buttonBar->addStretch();

    mSaveButton = new QPushButton("Export Settings...");
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

    // Load Global Variables (GLOB) - water related
    auto& globCollection = mData->getGlobCollection();
    auto globRecords = globCollection.getRecords();

    QTreeWidgetItem* waterGroup = new QTreeWidgetItem(mTree);
    waterGroup->setText(0, "Water Settings");
    waterGroup->setText(1, "GROUP");
    waterGroup->setText(2, "Water height, color, and behavior settings");

    int waterCount = 0;

    for (const auto& record : globRecords) {
        if (record.state == State_Erased) continue;

        const GlobalVariable& glob = record.get();
        QString name = glob.editorId;
        QString value;

        // Convert variant to string
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

        // Categorize based on name patterns
        bool isWater = name.contains("water", Qt::CaseInsensitive) ||
                       name.contains("ocean", Qt::CaseInsensitive) ||
                       name.contains("lake", Qt::CaseInsensitive) ||
                       name.contains("river", Qt::CaseInsensitive) ||
                       name.contains("sea", Qt::CaseInsensitive);

        if (isWater) {
            QTreeWidgetItem* item = new QTreeWidgetItem(waterGroup);
            if (item) {
                item->setText(0, name);
                item->setText(1, "GLOB");
                item->setText(2, value);
                item->setData(0, Qt::UserRole, name);
                item->setData(0, Qt::UserRole + 1, value);
                item->setData(0, Qt::UserRole + 2, QStringLiteral("GLOB"));
                waterCount++;
            }
        }
    }

    // Load Game Settings (GMST) - water related
    auto& gmstCollection = mData->getGameSettings();
    auto gmstRecords = gmstCollection.getRecords();

    for (const auto& record : gmstRecords) {
        if (record.state == State_Erased) continue;

        const GameSetting& gmst = record.get();
        QString name = gmst.editorId;
        QString value;

        // Convert variant to string
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

        // Categorize based on name patterns
        bool isWater = name.contains("water", Qt::CaseInsensitive) ||
                       name.contains("ocean", Qt::CaseInsensitive) ||
                       name.contains("lake", Qt::CaseInsensitive) ||
                       name.contains("river", Qt::CaseInsensitive) ||
                       name.contains("sea", Qt::CaseInsensitive);

        if (isWater) {
            QTreeWidgetItem* item = new QTreeWidgetItem(waterGroup);
            if (item) {
                item->setText(0, name);
                item->setText(1, "GMST");
                item->setText(2, value);
                item->setData(0, Qt::UserRole, name);
                item->setData(0, Qt::UserRole + 1, value);
                item->setData(0, Qt::UserRole + 2, QStringLiteral("GMST"));
                waterCount++;
            }
        }
    }

    waterGroup->setText(2, QString("Water Settings (%1 settings)").arg(waterCount));
    mTree->expandAll();
    mStatusLabel->setText(QString("Loaded %1 water settings").arg(waterCount));
    LOG_INFO(QString("Loaded %1 water settings").arg(waterCount));
}

void WaterEditor::refreshTree()
{
    loadSettings();
}

void WaterEditor::onNodeSelected(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);

    if (!item) return;

    mEditButton->setEnabled(true);
    mDeleteButton->setEnabled(true);

    QString name = item->data(0, Qt::UserRole).toString();
    QString value = item->data(0, Qt::UserRole + 1).toString();
    QString type = item->data(0, Qt::UserRole + 2).toString();

    if (!name.isEmpty()) {
        mSelectedName = name;
        mSelectedValue = value;
        mSelectedType = type;
        showSettingDetails(name, value);
    }
}

void WaterEditor::showSettingDetails(const QString& name, const QString& value)
{
    QString text;
    text += QString("<h2>%1</h2>").arg(name);
    text += QString("<p><b>Current Value:</b> %1</p>").arg(value);
    text += "<hr>";
    text += "<p><b>Description:</b></p>";
    text += "<p>Water setting that controls water behavior, height, color, or physics.</p>";

    mDetailEdit->setHtml(text);
}

void WaterEditor::onAddSetting()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, "Add Setting",
        "Enter setting name (e.g., fOceanWaveHeight):", QLineEdit::Normal, "", &ok);

    if (!ok || name.isEmpty()) return;

    QString value = QInputDialog::getText(this, "Set Value",
        "Enter initial value:", QLineEdit::Normal, "0.0", &ok);

    if (!ok) return;

    LOG_INFO(QString("Added setting '%1' with value '%2'").arg(name).arg(value));
    mStatusLabel->setText(QString("Added setting '%1'").arg(name));
    refreshTree();
}

void WaterEditor::onEditSetting()
{
    if (mSelectedName.isEmpty()) return;

    bool ok = false;
    QString newValue = QInputDialog::getText(this, "Edit Setting",
        QString("Enter new value for '%1':").arg(mSelectedName),
        QLineEdit::Normal, mSelectedValue, &ok);

    if (!ok) return;

    if (mSelectedType == QLatin1String("GLOB"))
    {
        auto& coll = mData->getGlobCollection();
        int idx = coll.searchId(mSelectedName);
        if (idx < 0)
        {
            LOG_WARNING(QString("Cannot edit global '%1': not found in globals").arg(mSelectedName));
            return;
        }

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
    }
    else
    {
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
    }

    mSelectedValue = newValue;
    LOG_INFO(QString("Updated setting '%1' to '%2'").arg(mSelectedName).arg(newValue));
    refreshTree();
}

void WaterEditor::onDeleteSetting()
{
    if (mSelectedName.isEmpty()) return;

    auto reply = QMessageBox::question(this, "Delete Setting",
        QString("Are you sure you want to delete setting '%1'?\n\nThis action cannot be undone.")
            .arg(mSelectedName),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        bool removed = false;
        if (mSelectedType == QLatin1String("GLOB"))
        {
            removed = mData->getGlobCollection().removeRecordWithUndo(mSelectedName, mData->getUndoStack());
        }
        else
        {
            removed = mData->getGameSettings().removeRecordWithUndo(mSelectedName, mData->getUndoStack());
        }

        if (removed)
        {
            LOG_INFO(QString("Deleted setting '%1'").arg(mSelectedName));
        }
        else
        {
            LOG_WARNING(QString("Could not delete setting '%1': not found").arg(mSelectedName));
        }
        mSelectedName.clear();
        mSelectedValue.clear();
        mSelectedType.clear();
        refreshTree();
    }
}

void WaterEditor::onSave()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save Water Settings", "",
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

    int globCount = 0;
    int gmstCount = 0;

    // Save Global Variables
    auto& globCollection = mData->getGlobCollection();
    auto globRecords = globCollection.getRecords();

    for (const auto& record : globRecords) {
        if (record.state == State_Erased) continue;

        const GlobalVariable& glob = record.get();
        QString name = glob.editorId;

        bool isWater = name.contains("water", Qt::CaseInsensitive) ||
                       name.contains("ocean", Qt::CaseInsensitive) ||
                       name.contains("lake", Qt::CaseInsensitive) ||
                       name.contains("river", Qt::CaseInsensitive) ||
                       name.contains("sea", Qt::CaseInsensitive);

        if (isWater) {
            writer.startRecord('GLOB');
            glob.save(writer);
            writer.endRecord();
            globCount++;
        }
    }

    // Save Game Settings
    auto& gmstCollection = mData->getGameSettings();
    auto gmstRecords = gmstCollection.getRecords();

    for (const auto& record : gmstRecords) {
        if (record.state == State_Erased) continue;

        const GameSetting& gmst = record.get();
        QString name = gmst.editorId;

        bool isWater = name.contains("water", Qt::CaseInsensitive) ||
                       name.contains("ocean", Qt::CaseInsensitive) ||
                       name.contains("lake", Qt::CaseInsensitive) ||
                       name.contains("river", Qt::CaseInsensitive) ||
                       name.contains("sea", Qt::CaseInsensitive);

        if (isWater) {
            writer.startRecord('GMST');
            gmst.save(writer);
            writer.endRecord();
            gmstCount++;
        }
    }

    file.close();

    int totalCount = globCount + gmstCount;
    LOG_INFO(QString("Saved %1 GLOB, %2 GMST water settings to %3")
        .arg(globCount).arg(gmstCount).arg(filePath));

    QMessageBox::information(this, "Saved",
        QString("Water settings saved.\n\n"
                "Global Variables: %1\n"
                "Game Settings: %2\n"
                "Total: %3\n\n"
                "File: %4")
            .arg(globCount)
            .arg(gmstCount)
            .arg(totalCount)
            .arg(filePath));
}
