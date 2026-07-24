#include "exportdialog.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/world/ckid.hpp"
#include "../../../libs/files/data/dataexporter.hpp"
#include "../../../libs/files/data/dataimporter.hpp"
#include "dialrecord.hpp"
#include "inforecord.hpp"
#include "npcrecord.hpp"
#include "logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QPushButton>
#include <QFileDialog>
#include <QComboBox>
#include <QListWidget>
#include <QCheckBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QHeaderView>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

ImportPreviewDialog::ImportPreviewDialog(const QList<ImportPreviewInfo>& records, const QString& filePath, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Import Preview");
    setMinimumSize(450, 350);

    auto* layout = new QVBoxLayout(this);

    auto* fileLabel = new QLabel(QString("File: %1").arg(filePath), this);
    fileLabel->setWordWrap(true);
    layout->addWidget(fileLabel);

    auto* table = new QTableWidget(this);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({"Record Type", "Count", "Action"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);

    int totalRecords = 0;
    int newCount = 0;
    int updateCount = 0;

    for (const auto& info : records)
    {
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(info.recordType));
        table->setItem(row, 1, new QTableWidgetItem(QString::number(info.count)));
        table->setItem(row, 2, new QTableWidgetItem(info.action));
        totalRecords += info.count;
        if (info.action == "New")
            newCount += info.count;
        else
            updateCount += info.count;
    }

    table->resizeColumnsToContents();
    layout->addWidget(table, 1);

    auto* summaryLabel = new QLabel(
        QString("Total: %1 records (%2 new, %3 updates)")
            .arg(totalRecords).arg(newCount).arg(updateCount), this);
    layout->addWidget(summaryLabel);

    auto* btnLayout = new QHBoxLayout();
    auto* okBtn = new QPushButton("Import", this);
    auto* cancelBtn = new QPushButton("Cancel", this);
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

ExportDialog::ExportDialog(Data* data, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mLogEdit(nullptr)
{
    setupUI();
}

void ExportDialog::setupUI()
{
    setWindowTitle("Export / Import");
    setMinimumSize(600, 500);

    auto* mainLayout = new QVBoxLayout(this);

    // Record type selection
    auto* typeGroup = new QGroupBox("Record Types to Export", this);
    auto* typeLayout = new QVBoxLayout(typeGroup);
    
    mTypeList = new QListWidget();
    QStringList typeNames = {
        "NPC_", "WEAP_", "ARMOR_", "SPEL_", "MGEF", "QUST_",
        "DIAL_", "INFO_", "PACK_", "ALCH_", "INGR_", "CONT_",
        "ENCH_", "BOOK_", "MISC_", "ACTI_", "STAT_", "RACE_",
        "CLASS_", "FACT_", "PERK_", "CEL_", "WRLD_", "LOCT_", "REFR_"
    };
    
    for (const QString& typeName : typeNames)
    {
        auto* item = new QListWidgetItem(typeName, mTypeList);
        item->setCheckState(Qt::Checked);
    }
    
    typeLayout->addWidget(mTypeList);
    mainLayout->addWidget(typeGroup);

    // Export filter
    auto* filterGroup = new QGroupBox("Export Filter", this);
    auto* filterLayout = new QGridLayout(filterGroup);

    filterLayout->addWidget(new QLabel("Editor ID Pattern (regex):"), 0, 0);
    mEditorIdFilter = new QLineEdit();
    mEditorIdFilter->setPlaceholderText("Leave empty for no filter");
    filterLayout->addWidget(mEditorIdFilter, 0, 1);

    filterLayout->addWidget(new QLabel("Form ID Range:"), 1, 0);
    auto* formIdLayout = new QHBoxLayout();
    mFormIdMin = new QSpinBox();
    mFormIdMin->setRange(0, 0x7FFFFFFF);
    mFormIdMin->setValue(0);
    mFormIdMax = new QSpinBox();
    mFormIdMax->setRange(0, 0x7FFFFFFF);
    mFormIdMax->setValue(0x7FFFFFFF);
    formIdLayout->addWidget(mFormIdMin);
    formIdLayout->addWidget(new QLabel(" to "));
    formIdLayout->addWidget(mFormIdMax);
    formIdLayout->addStretch();
    filterLayout->addLayout(formIdLayout, 1, 1);

    mOnlyModified = new QCheckBox("Export only modified records");
    mOnlyModified->setChecked(true);
    filterLayout->addWidget(mOnlyModified, 2, 0, 1, 2);

    mOnlyDeleted = new QCheckBox("Export only deleted records");
    filterLayout->addWidget(mOnlyDeleted, 3, 0, 1, 2);

    mainLayout->addWidget(filterGroup);

    // Export format selection
    auto* formatGroup = new QGroupBox("Export Format", this);
    auto* formatLayout = new QHBoxLayout(formatGroup);
    
    mFormatCombo = new QComboBox(this);
    mFormatCombo->addItems({"JSON", "CSV", "XML"});
    formatLayout->addWidget(new QLabel("Format:"));
    formatLayout->addWidget(mFormatCombo);
    formatLayout->addStretch();
    
    mainLayout->addWidget(formatGroup);

    // Log area
    mLogEdit = new QPlainTextEdit();
    mLogEdit->setReadOnly(true);
    mLogEdit->setPlaceholderText("Export/import log will appear here...");
    mainLayout->addWidget(mLogEdit, 1);

    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    
    auto* exportBtn = new QPushButton("Export Selected", this);
    auto* importBtn = new QPushButton("Import Data", this);
    auto* closeBtn = new QPushButton("Close", this);
    
    buttonLayout->addWidget(exportBtn);
    buttonLayout->addWidget(importBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);
    
    mainLayout->addLayout(buttonLayout);

    connect(exportBtn, &QPushButton::clicked, this, &ExportDialog::onExportClicked);
    connect(importBtn, &QPushButton::clicked, this, &ExportDialog::onImportClicked);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

QString ExportDialog::selectExportDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Export Directory");
    return dir;
}

QString ExportDialog::selectExportFile(const QString& filter)
{
    QString file = QFileDialog::getSaveFileName(this, "Save Export File", "", filter);
    return file;
}

QStringList ExportDialog::selectImportFiles(const QString& filter)
{
    QStringList files = QFileDialog::getOpenFileNames(this, "Select Import Files", "", filter);
    return files;
}

void ExportDialog::logMessage(const QString& message)
{
    mLogEdit->appendPlainText(message);
}

void ExportDialog::onExportClicked()
{
    exportGenericData();
}

void ExportDialog::onImportClicked()
{
    importData();
}

void ExportDialog::exportDialogue()
{
    LOG_INFO("Exporting dialogue to text files");

    QString exportDir = selectExportDir();
    if (exportDir.isEmpty())
    {
        LOG_INFO("Export cancelled - no directory selected");
        return;
    }

    auto& dialogs = mData->getDialCollection();
    auto& infos = mData->getInfoCollection();

    int exported = 0;
    int total = dialogs.size();

    for (int i = 0; i < total; i++)
    {
        const DialRecord& dial = dialogs.getRecord(i).get();
        writeDialogueTopic(exportDir, dial, infos);
        exported++;
        LOG_DEBUG(QString("Exported dialogue topic '%1'").arg(dial.editorId));
    }

    QString logMsg = QString("Exported %1/%2 dialogue topics to: %3")
                         .arg(exported).arg(total).arg(exportDir);
    mLogEdit->appendPlainText(logMsg);
    LOG_INFO(logMsg);
}

void ExportDialog::exportScripts()
{
    LOG_INFO("Exporting scripts to text files");

    QString exportDir = selectExportDir();
    if (exportDir.isEmpty())
    {
        LOG_INFO("Export cancelled - no directory selected");
        return;
    }

    int exported = 0;

    auto& quests = mData->getQuestCollection();
    for (int i = 0; i < quests.size(); i++)
    {
        const QuestRecord& quest = quests.getRecord(i).get();
        for (int j = 0; j < quest.scriptIds.size(); j++)
        {
            QString scriptPath = QDir(exportDir).filePath(quest.editorId + "_script" + QString::number(j) + ".psc");
            QFile file(scriptPath);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                LOG_WARNING(QString("Failed to create script file for quest '%1'").arg(quest.editorId));
                continue;
            }

            QTextStream out(&file);
            out << "; Papyrus script from quest: " << quest.editorId << "\n";
            out << "; Script ID: 0x" << QString::number(quest.scriptIds[j], 16).toUpper().rightJustified(8, '0') << "\n";
            out << "ScriptName " << quest.editorId << "Script\n\n";
            out << "EndScript\n";
            file.close();
            exported++;
        }
    }

    auto& infos = mData->getInfoCollection();
    for (int i = 0; i < infos.size(); i++)
    {
        const InfoRecord& info = infos.getRecord(i).get();
        for (int j = 0; j < info.scriptIds.size(); j++)
        {
            QString scriptPath = QDir(exportDir).filePath(info.editorId + "_dialogue_script" + QString::number(j) + ".psc");
            QFile file(scriptPath);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                LOG_WARNING(QString("Failed to create script file for dialog info '%1'").arg(info.editorId));
                continue;
            }

            QTextStream out(&file);
            out << "; Papyrus script from dialog info: " << info.editorId << "\n";
            out << "; Script ID: 0x" << QString::number(info.scriptIds[j], 16).toUpper().rightJustified(8, '0') << "\n";
            out << "ScriptName " << info.editorId << "Script\n\n";
            out << "EndScript\n";
            file.close();
            exported++;
        }
    }

    QString logMsg = QString("Exported %1 scripts to: %2").arg(exported).arg(exportDir);
    mLogEdit->appendPlainText(logMsg);
    LOG_INFO(logMsg);
}

void ExportDialog::exportTextures()
{
    LOG_INFO("Exporting NPC face texture paths");

    QString exportDir = selectExportDir();
    if (exportDir.isEmpty())
    {
        LOG_INFO("Export cancelled - no directory selected");
        return;
    }

    auto& npcs = mData->getNpcCollection();
    int total = npcs.size();
    int withTextures = 0;

    QString manifestPath = QDir(exportDir).filePath("npc_face_manifest.txt");
    QFile manifest(manifestPath);
    if (!manifest.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        mLogEdit->appendPlainText("Failed to create manifest file.");
        LOG_WARNING("Failed to create NPC face manifest file");
        return;
    }

    QTextStream out(&manifest);
    out << "NPC Face Texture Manifest\n";
    out << "=========================\n\n";
    out << "This file lists the face texture paths for all NPCs in the plugin.\n\n";

    for (int i = 0; i < total; i++)
    {
        const NpcRecord& npc = npcs.getRecord(i).get();
        if (!npc.fullName.isEmpty() || !npc.editorId.isEmpty())
        {
            writeManifestEntry(out, npc);
            withTextures++;
        }
    }
    manifest.close();

    QString logMsg = QString("Exported face texture paths for %1/%2 NPCs to: %3")
                         .arg(withTextures).arg(total).arg(exportDir);
    mLogEdit->appendPlainText(logMsg);
    LOG_INFO(logMsg);
}

void ExportDialog::exportGenericData()
{
    QListWidgetItem* firstItem = mTypeList->item(0);
    if (!firstItem || firstItem->checkState() != Qt::Checked)
    {
        QMessageBox::warning(this, "No Selection", "Please select at least one record type to export.");
        return;
    }

    QStringList selectedTypes;
    for (int i = 0; i < mTypeList->count(); i++)
    {
        if (mTypeList->item(i)->checkState() == Qt::Checked)
        {
            selectedTypes.append(mTypeList->item(i)->text());
        }
    }

    if (selectedTypes.isEmpty())
    {
        QMessageBox::warning(this, "No Selection", "Please select at least one record type to export.");
        return;
    }

    QString format = mFormatCombo->currentText();
    QString filePath;
    
    if (format == "JSON")
    {
        filePath = selectExportFile("JSON Files (*.json)");
    }
    else if (format == "CSV")
    {
        filePath = selectExportFile("CSV Files (*.csv)");
    }
    else if (format == "XML")
    {
        filePath = selectExportFile("XML Files (*.xml)");
    }

    if (filePath.isEmpty())
    {
        logMessage("Export cancelled - no file selected");
        return;
    }

    DataExporter::ExportFilter filter;
    filter.editorIdPattern = mEditorIdFilter->text();
    filter.formIdMin = static_cast<quint32>(mFormIdMin->value());
    filter.formIdMax = static_cast<quint32>(mFormIdMax->value());
    filter.onlyModified = mOnlyModified->isChecked();
    filter.onlyDeleted = mOnlyDeleted->isChecked();

    logMessage("--- Export Filter ---");
    if (!filter.editorIdPattern.isEmpty())
        logMessage(QString("  Editor ID pattern: %1").arg(filter.editorIdPattern));
    logMessage(QString("  Form ID range: 0x%1 - 0x%2")
                   .arg(filter.formIdMin, 8, 16, QChar('0'))
                   .arg(filter.formIdMax, 8, 16, QChar('0')));
    logMessage(QString("  Only modified: %1").arg(filter.onlyModified ? "Yes" : "No"));
    logMessage(QString("  Only deleted: %1").arg(filter.onlyDeleted ? "Yes" : "No"));

    DataExporter::ExportResult result;
    
    if (format == "JSON")
    {
        result = DataExporter::exportToJSON(*mData, selectedTypes, filePath, filter);
    }
    else if (format == "CSV")
    {
        result = DataExporter::exportToCSV(*mData, selectedTypes, filePath, filter);
    }
    else if (format == "XML")
    {
        result = DataExporter::exportToXML(*mData, selectedTypes, filePath, filter);
    }

    if (!result.error.isEmpty())
    {
        logMessage(QString("Export failed: %1").arg(result.error));
        QMessageBox::critical(this, "Export Error", result.error);
    }
    else
    {
        logMessage("--- Export Statistics ---");
        logMessage(QString("Total records exported: %1").arg(result.recordsExported));
        if (!result.recordsByType.isEmpty())
        {
            logMessage("Records by type:");
            for (auto it = result.recordsByType.constBegin(); it != result.recordsByType.constEnd(); ++it)
            {
                logMessage(QString("  %1: %2").arg(it.key()).arg(it.value()));
            }
        }
        logMessage(QString("Output file: %1").arg(result.outputPath));
        logMessage(QString("File size: %1 bytes").arg(result.fileSize));
        logMessage(QString("Format: %1").arg(format));
        logMessage("--- Export Complete ---");

        QString summaryMsg = QString("Exported %1 records to %2 (%3, %4 bytes)")
                                 .arg(result.recordsExported).arg(filePath).arg(format).arg(result.fileSize);
        QMessageBox::information(this, "Export Complete", summaryMsg);
    }
}

QList<ImportPreviewInfo> ExportDialog::scanImportFile(const QString& filePath)
{
    QList<ImportPreviewInfo> records;
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return records;
    }
    QByteArray contents = file.readAll();
    file.close();

    if (suffix == "json")
    {
        QJsonDocument doc = QJsonDocument::fromJson(contents);
        if (!doc.isObject())
            return records;

        QJsonObject root = doc.object();
        QStringList typeKeys = {
            "NPC_", "WEAP_", "ARMOR_", "SPEL_", "MGEF", "QUST_",
            "DIAL_", "INFO_", "PACK_", "ALCH_", "INGR_", "CONT_",
            "ENCH_", "BOOK_", "MISC_", "ACTI_", "STAT_", "RACE_",
            "CLASS_", "FACT_", "PERK_", "CELL", "WRLD_", "LOCT_", "REFR_"
        };

        for (const QString& key : typeKeys)
        {
            if (root.contains(key))
            {
                QJsonValue val = root.value(key);
                int count = 0;
                if (val.isArray())
                    count = val.toArray().size();
                else if (val.isObject())
                    count = 1;

                if (count > 0)
                {
                    ImportPreviewInfo info;
                    info.recordType = key;
                    info.count = count;
                    info.action = "New";
                    records.append(info);
                }
            }
        }
    }
    else if (suffix == "csv" || suffix == "xml")
    {
        int lineCount = 0;
        QTextStream stream(contents);
        while (!stream.atEnd())
        {
            QString line = stream.readLine().trimmed();
            if (!line.isEmpty() && line != "---")
                lineCount++;
        }

        if (lineCount > 0)
        {
            ImportPreviewInfo info;
            info.recordType = suffix.toUpper();
            info.count = lineCount;
            info.action = "New";
            records.append(info);
        }
    }

    return records;
}

void ExportDialog::importData()
{
    QStringList files = selectImportFiles("JSON Files (*.json);;CSV Files (*.csv);;XML Files (*.xml)");
    if (files.isEmpty())
    {
        logMessage("Import cancelled - no files selected");
        return;
    }

    QList<ImportPreviewInfo> allRecords;
    for (const QString& filePath : files)
    {
        QList<ImportPreviewInfo> fileRecords = scanImportFile(filePath);
        for (const ImportPreviewInfo& info : fileRecords)
        {
            bool found = false;
            for (int i = 0; i < allRecords.size(); i++)
            {
                if (allRecords[i].recordType == info.recordType)
                {
                    allRecords[i].count += info.count;
                    found = true;
                    break;
                }
            }
            if (!found)
                allRecords.append(info);
        }
    }

    if (allRecords.isEmpty())
    {
        logMessage("No importable records found in selected files.");
        QMessageBox::warning(this, "Import", "No importable records found in the selected files.");
        return;
    }

    QString fileListStr = files.join(", ");
    if (files.size() == 1)
        fileListStr = files.first();

    ImportPreviewDialog preview(allRecords, fileListStr, this);
    if (preview.exec() != QDialog::Accepted)
    {
        logMessage("Import cancelled by user.");
        return;
    }

    int totalImported = 0;
    int totalSkipped = 0;
    QStringList allWarnings;

    for (const QString& filePath : files)
    {
        QFileInfo fileInfo(filePath);
        QString suffix = fileInfo.suffix().toLower();

        DataImporter::ImportResult result;

        if (suffix == "json")
            result = DataImporter::importFromJSON(*mData, filePath);
        else if (suffix == "csv")
            result = DataImporter::importFromCSV(*mData, filePath);
        else if (suffix == "xml")
            result = DataImporter::importFromXML(*mData, filePath);
        else
        {
            logMessage(QString("Skipping unsupported file: %1").arg(filePath));
            continue;
        }

        if (!result.error.isEmpty())
        {
            logMessage(QString("Import failed for %1: %2").arg(filePath, result.error));
            QMessageBox::critical(this, "Import Error", QString("%1:\n%2").arg(filePath, result.error));
            return;
        }

        totalImported += result.recordsImported;
        totalSkipped += result.recordsSkipped;
        allWarnings.append(result.warnings);

        logMessage(QString("Imported %1 records from %2").arg(result.recordsImported).arg(filePath));
    }

    QString logMsg = QString("Batch import complete: %1 records imported, %2 skipped from %3 file(s)")
                         .arg(totalImported).arg(totalSkipped).arg(files.size());
    if (!allWarnings.isEmpty())
    {
        logMsg += QString("\nWarnings: %1").arg(allWarnings.join("; "));
    }
    logMessage(logMsg);
    QMessageBox::information(this, "Import Complete", logMsg);
}

void ExportDialog::writeManifestEntry(QTextStream& out, const NpcRecord& npc)
{
    out << "---\n";
    out << "EditorID: " << npc.editorId << "\n";
    out << "Full Name: " << npc.fullName << "\n";
    out << "FormID: 0x" << QString::number(npc.formId, 16).toUpper().rightJustified(8, '0') << "\n";

    QString texPath;
    for (int i = 0; i < npc.rawSubRecords.size(); i++)
    {
        const RawSubRecord& sub = npc.rawSubRecords[i];
        QString subName = QString::fromUtf8(reinterpret_cast<const char*>(&sub.name), 4);
        if (subName == "BOD3" || subName == "BNAM")
        {
            QByteArray data = sub.data;
            int nullPos = data.indexOf('\0');
            if (nullPos >= 0)
            {
                QString path = QString::fromUtf8(data.left(nullPos).constData());
                if (!path.isEmpty())
                {
                    texPath = path;
                    break;
                }
            }
        }
    }

    if (texPath.isEmpty())
    {
        texPath = "Characters\\N\\1\\";
    }

    out << "Face Texture Path: " << texPath << "\n";
    out << "---\n";
}

void ExportDialog::writeDialogueTopic(const QString& exportDir, const DialRecord& dial, const IdCollection<InfoRecord>& infos)
{
    QString diaDir = QDir(exportDir).filePath(dial.editorId);
    QDir().mkpath(diaDir);

    QString filePath = QDir(diaDir).filePath("topic.txt");
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        LOG_WARNING(QString("Failed to create dialogue file for topic '%1'").arg(dial.editorId));
        return;
    }

    QTextStream out(&file);
    out << "Topic: " << dial.topicName << "\n\n";
    out << "EditorID: " << dial.editorId << "\n";
    out << "FormID: 0x" << QString::number(dial.formId, 16).toUpper().rightJustified(8, '0') << "\n\n";

    if (dial.responseIds.isEmpty())
    {
        out << "No responses defined for this topic.\n";
    }
    else
    {
        for (quint32 infoId : dial.responseIds)
        {
            QString infoIdStr = QString::number(infoId, 16).toUpper().rightJustified(8, '0');
            bool found = false;
            for (int j = 0; j < infos.size(); j++)
            {
                const InfoRecord& info = infos.getRecord(j).get();
                if (QString::number(info.formId, 16).toUpper().rightJustified(8, '0') == infoIdStr)
                {
                    out << "Response: " << info.responseText << "\n";
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                out << "Response: [Info record not found: 0x" << infoIdStr << "]\n";
            }
            out << "---\n";
        }
    }

    file.close();
}
