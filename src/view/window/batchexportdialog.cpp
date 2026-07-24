#include "batchexportdialog.hpp"

#include <QtWidgets>

#include "../../model/world/data.hpp"
#include "../../model/tools/assetconverter.hpp"
#include "../../../libs/files/data/dataexporter.hpp"
#include "../../../libs/files/data/dataimporter.hpp"
#include "exporttemplatesdialog.hpp"
#include "logger.hpp"

BatchExportDialog::BatchExportDialog(Data* data, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mRecordTypeTable(nullptr),
      mRecordFormatCombo(nullptr),
      mTemplateCombo(nullptr),
      mEditorIdFilter(nullptr),
      mFormIdMin(nullptr),
      mFormIdMax(nullptr),
      mOnlyModified(nullptr),
      mOnlyDeleted(nullptr),
      mExportRecordsBtn(nullptr),
      mAssetFileList(nullptr),
      mAssetTypeCombo(nullptr),
      mAssetTargetFormatCombo(nullptr),
      mAddFilesBtn(nullptr),
      mRemoveFilesBtn(nullptr),
      mClearFilesBtn(nullptr),
      mConvertAssetsBtn(nullptr),
      mLogEdit(nullptr),
      mProgress(nullptr)
{
    setWindowTitle("Batch Export");
    setMinimumSize(800, 600);
    setupUI();
}

void BatchExportDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // Tab widget for Record Export and Asset Conversion
    auto* tabWidget = new QTabWidget(this);

    // ── Tab 1: Record Data Export ──────────────────────────────────────────
    auto* recordTab = new QWidget();
    auto* recordLayout = new QVBoxLayout(recordTab);

    // Record type selection table
    auto* typeGroup = new QGroupBox("Record Types", recordTab);
    auto* typeLayout = new QVBoxLayout(typeGroup);
    
    mRecordTypeTable = new QTableWidget();
    mRecordTypeTable->setColumnCount(3);
    mRecordTypeTable->setHorizontalHeaderLabels({"Record Type", "Available", "Selected"});
    mRecordTypeTable->horizontalHeader()->setStretchLastSection(true);
    mRecordTypeTable->setSelectionBehavior(QTableWidget::SelectRows);
    mRecordTypeTable->setEditTriggers(QTableWidget::NoEditTriggers);
    mRecordTypeTable->setAlternatingRowColors(true);
    typeLayout->addWidget(mRecordTypeTable);
    recordLayout->addWidget(typeGroup);

    // Export options
    auto* optionsGroup = new QGroupBox("Export Options", recordTab);
    auto* optionsLayout = new QGridLayout(optionsGroup);

    optionsLayout->addWidget(new QLabel("Format:"), 0, 0);
    mRecordFormatCombo = new QComboBox();
    mRecordFormatCombo->addItems({"JSON", "CSV", "XML"});
    optionsLayout->addWidget(mRecordFormatCombo, 0, 1);

    optionsLayout->addWidget(new QLabel("Template (CSV):"), 1, 0);
    mTemplateCombo = new QComboBox();
    mTemplateCombo->addItem("(Default)");
    populateRecordTypes();
    optionsLayout->addWidget(mTemplateCombo, 1, 1);

    optionsLayout->addWidget(new QLabel("Editor ID Filter (regex):"), 2, 0);
    mEditorIdFilter = new QLineEdit();
    mEditorIdFilter->setPlaceholderText("e.g., ^FC_.*");
    optionsLayout->addWidget(mEditorIdFilter, 2, 1);

    optionsLayout->addWidget(new QLabel("Form ID Range:"), 3, 0);
    auto* formIdLayout = new QHBoxLayout();
    mFormIdMin = new QSpinBox();
    mFormIdMin->setRange(0, 0xFFFFFFFF);
    mFormIdMin->setValue(0);
    mFormIdMin->setSuffix(" (0x0)");
    formIdLayout->addWidget(mFormIdMin);
    formIdLayout->addWidget(new QLabel(" to "));
    mFormIdMax = new QSpinBox();
    mFormIdMax->setRange(0, 0xFFFFFFFF);
    mFormIdMax->setValue(0x7FFFFFFF);
    mFormIdMax->setSuffix(" (0x7FFFFFFF)");
    formIdLayout->addWidget(mFormIdMax);
    optionsLayout->addLayout(formIdLayout, 3, 1);

    mOnlyModified = new QCheckBox("Only modified records");
    mOnlyModified->setChecked(true);
    optionsLayout->addWidget(mOnlyModified, 4, 0, 1, 2);

    mOnlyDeleted = new QCheckBox("Only deleted records");
    optionsLayout->addWidget(mOnlyDeleted, 5, 0, 1, 2);

    recordLayout->addWidget(optionsGroup);

    // Export button
    mExportRecordsBtn = new QPushButton("Export Selected Records");
    mExportRecordsBtn->setStyleSheet("QPushButton { padding: 8px; font-weight: bold; }");
    recordLayout->addWidget(mExportRecordsBtn);
    recordLayout->addStretch();

    tabWidget->addTab(recordTab, "Record Data Export");

    // ── Tab 2: Asset Conversion ────────────────────────────────────────────
    auto* assetTab = new QWidget();
    auto* assetLayout = new QVBoxLayout(assetTab);

    // File list
    auto* fileGroup = new QGroupBox("Files to Convert", assetTab);
    auto* fileListLayout = new QVBoxLayout(fileGroup);
    
    mAssetFileList = new QListWidget();
    mAssetFileList->setMinimumHeight(200);
    mAssetFileList->setSelectionMode(QAbstractItemView::MultiSelection);
    fileListLayout->addWidget(mAssetFileList);

    auto* fileBtnLayout = new QHBoxLayout();
    mAddFilesBtn = new QPushButton("Add Files...");
    mRemoveFilesBtn = new QPushButton("Remove Selected");
    mClearFilesBtn = new QPushButton("Clear All");
    fileBtnLayout->addWidget(mAddFilesBtn);
    fileBtnLayout->addWidget(mRemoveFilesBtn);
    fileBtnLayout->addWidget(mClearFilesBtn);
    fileBtnLayout->addStretch();
    fileListLayout->addLayout(fileBtnLayout);

    assetLayout->addWidget(fileGroup);

    // Conversion options
    auto* convertGroup = new QGroupBox("Conversion Options", assetTab);
    auto* convertLayout = new QGridLayout(convertGroup);

    convertLayout->addWidget(new QLabel("Asset Type:"), 0, 0);
    mAssetTypeCombo = new QComboBox();
    mAssetTypeCombo->addItems({"NIF Models", "Textures", "Sounds"});
    connect(mAssetTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BatchExportDialog::populateAssetTypes);
    convertLayout->addWidget(mAssetTypeCombo, 0, 1);

    convertLayout->addWidget(new QLabel("Target Format:"), 1, 0);
    mAssetTargetFormatCombo = new QComboBox();
    populateAssetTypes();
    convertLayout->addWidget(mAssetTargetFormatCombo, 1, 1);

    assetLayout->addWidget(convertGroup);

    // Convert button
    mConvertAssetsBtn = new QPushButton("Convert Assets");
    mConvertAssetsBtn->setStyleSheet("QPushButton { padding: 8px; font-weight: bold; }");
    assetLayout->addWidget(mConvertAssetsBtn);
    assetLayout->addStretch();

    tabWidget->addTab(assetTab, "Asset Conversion");

    mainLayout->addWidget(tabWidget);

    // Log area
    auto* logGroup = new QGroupBox("Export Log", this);
    auto* logLayout = new QVBoxLayout(logGroup);
    
    mLogEdit = new QPlainTextEdit();
    mLogEdit->setReadOnly(true);
    mLogEdit->setPlaceholderText("Export log will appear here...");
    logLayout->addWidget(mLogEdit);

    mainLayout->addWidget(logGroup, 1);

    // Connections
    connect(mExportRecordsBtn, &QPushButton::clicked, this, &BatchExportDialog::onExportRecords);
    connect(mConvertAssetsBtn, &QPushButton::clicked, this, &BatchExportDialog::onConvertAssets);
    connect(mAddFilesBtn, &QPushButton::clicked, this, &BatchExportDialog::onAddFiles);
    connect(mRemoveFilesBtn, &QPushButton::clicked, this, &BatchExportDialog::onRemoveFiles);
    connect(mClearFilesBtn, &QPushButton::clicked, this, &BatchExportDialog::onClearFiles);
    connect(mTemplateCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, &BatchExportDialog::onTemplateSelected);

    mProgress = new QProgressDialog(this);
    mProgress->setWindowModality(Qt::WindowModal);
    mProgress->setLabelText("Preparing...");
    mProgress->setCancelButton(nullptr);
    mProgress->setMinimumDuration(0);
    mProgress->hide();
}

void BatchExportDialog::populateRecordTypes()
{
    if (!mData) {
        return;
    }

    struct TypeInfo {
        QString name;
        CkId::Type type;
        QString (*countFunc)(const Data*);
    };

    QVector<TypeInfo> types = {
        {"NPC_", CkId::Type_Npc_, [](const Data* d) { return QString::number(d->getNpcCollection().size()); }},
        {"WEAP_", CkId::Type_Weap_, [](const Data* d) { return QString::number(d->getWeaponCollection().size()); }},
        {"ARMOR_", CkId::Type_Armor_, [](const Data* d) { return QString::number(d->getArmorCollection().size()); }},
        {"SPEL_", CkId::Type_Spel_, [](const Data* d) { return QString::number(d->getSpellCollection().size()); }},
        {"MGEF", CkId::Type_Magic_, [](const Data* d) { return QString::number(d->getMagicCollection().size()); }},
        {"QUST_", CkId::Type_Quest_, [](const Data* d) { return QString::number(d->getQuestCollection().size()); }},
        {"DIAL_", CkId::Type_Dial_, [](const Data* d) { return QString::number(d->getDialCollection().size()); }},
        {"INFO_", CkId::Type_Info_, [](const Data* d) { return QString::number(d->getInfoCollection().size()); }},
        {"PACK_", CkId::Type_Pack_, [](const Data* d) { return QString::number(d->getPackCollection().size()); }},
        {"ALCH_", CkId::Type_Alch_, [](const Data* d) { return QString::number(d->getAlchCollection().size()); }},
        {"INGR_", CkId::Type_Ingr_, [](const Data* d) { return QString::number(d->getIngrCollection().size()); }},
        {"CONT_", CkId::Type_Cont_, [](const Data* d) { return QString::number(d->getContCollection().size()); }},
        {"ENCH_", CkId::Type_Ench_, [](const Data* d) { return QString::number(d->getEnchCollection().size()); }},
        {"BOOK_", CkId::Type_Book_, [](const Data* d) { return QString::number(d->getBookCollection().size()); }},
        {"MISC_", CkId::Type_Misc_, [](const Data* d) { return QString::number(d->getMiscCollection().size()); }},
        {"ACTI_", CkId::Type_Acti_, [](const Data* d) { return QString::number(d->getActiCollection().size()); }},
        {"STAT_", CkId::Type_Stat_, [](const Data* d) { return QString::number(d->getStatCollection().size()); }},
        {"RACE_", CkId::Type_Race_, [](const Data* d) { return QString::number(d->getRaceCollection().size()); }},
        {"CLASS_", CkId::Type_Class_, [](const Data* d) { return QString::number(d->getClassCollection().size()); }},
        {"FACT_", CkId::Type_Fact_, [](const Data* d) { return QString::number(d->getFactCollection().size()); }},
        {"PERK_", CkId::Type_PerK_, [](const Data* d) { return QString::number(d->getPerkCollection().size()); }},
        {"CEL_", CkId::Type_Cel_, [](const Data* d) { return QString::number(d->getCellCollection().size()); }},
        {"WRLD_", CkId::Type_WRLD_, [](const Data* d) { return QString::number(d->getWorldspaceCollection().size()); }},
        {"LOCT_", CkId::Type_LOCT_, [](const Data* d) { return QString::number(d->getLocationCollection().size()); }},
        {"SOUN_", CkId::Type_Soun_, [](const Data* d) { return QString::number(d->getSounCollection().size()); }},
        {"WTHR_", CkId::Type_Wthr_, [](const Data* d) { return QString::number(d->getWthrCollection().size()); }},
        {"LTEX_", CkId::Type_Ltex_, [](const Data* d) { return QString::number(d->getLtexCollection().size()); }},
    };

    mRecordTypeTable->setRowCount(static_cast<int>(types.size()));

    for (int i = 0; i < types.size(); ++i) {
        auto* nameItem = new QTableWidgetItem(types[i].name);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsSelectable);
        mRecordTypeTable->setItem(i, 0, nameItem);

        auto* countItem = new QTableWidgetItem(types[i].countFunc(mData));
        countItem->setFlags(countItem->flags() & ~Qt::ItemIsSelectable);
        countItem->setTextAlignment(Qt::AlignCenter);
        mRecordTypeTable->setItem(i, 1, countItem);

        auto* checkItem = new QTableWidgetItem();
        checkItem->setCheckState(Qt::Checked);
        checkItem->setFlags(checkItem->flags() & ~Qt::ItemIsEditable);
        mRecordTypeTable->setItem(i, 2, checkItem);
    }

    // Load templates
    auto templates = TemplateManager::loadTemplates();
    for (const auto& t : templates) {
        mTemplateCombo->addItem(t.name);
    }
}

void BatchExportDialog::populateAssetTypes()
{
    int typeIdx = mAssetTypeCombo->currentIndex();
    mAssetTargetFormatCombo->clear();

    if (typeIdx == 0) { // NIF Models
        mAssetTargetFormatCombo->addItems({"OBJ"});
    } else if (typeIdx == 1) { // Textures
        mAssetTargetFormatCombo->addItems({"DDS", "TGA", "PNG"});
    } else { // Sounds
        mAssetTargetFormatCombo->addItem("OGG");
    }
}

void BatchExportDialog::onTemplateSelected()
{
}

void BatchExportDialog::onAddFiles()
{
    QString filter;
    int typeIdx = mAssetTypeCombo->currentIndex();
    
    if (typeIdx == 0) {
        filter = "NIF Files (*.nif);;All Files (*.*)";
    } else if (typeIdx == 1) {
        filter = "Texture Files (*.dds *.tga *.png *.bmp);;All Files (*.*)";
    } else {
        filter = "Sound Files (*.wav *.mp3);;All Files (*.*)";
    }

    QStringList files = QFileDialog::getOpenFileNames(this, "Select Files", "", filter);
    if (!files.isEmpty()) {
        mAssetFileList->addItems(files);
        logMessage(QString("Added %1 file(s)").arg(files.size()));
    }
}

void BatchExportDialog::onRemoveFiles()
{
    QList<QListWidgetItem*> items = mAssetFileList->selectedItems();
    for (QListWidgetItem* item : items) {
        delete mAssetFileList->takeItem(mAssetFileList->row(item));
    }
    logMessage(QString("Removed %1 file(s)").arg(items.size()));
}

void BatchExportDialog::onClearFiles()
{
    int count = mAssetFileList->count();
    mAssetFileList->clear();
    if (count > 0) {
        logMessage(QString("Cleared %1 file(s)").arg(count));
    }
}

void BatchExportDialog::logMessage(const QString& message)
{
    mLogEdit->appendPlainText(QString("[%1] %2")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
        .arg(message));
    mLogEdit->verticalScrollBar()->setValue(mLogEdit->verticalScrollBar()->maximum());
}

void BatchExportDialog::onExportRecords()
{
    if (!mData) {
        QMessageBox::warning(this, "No Data", "No plugin is currently loaded.\n\nOpen a plugin via File > Data first.");
        return;
    }

    // Collect selected record types
    QStringList selectedTypes;
    QVector<CkId::Type> selectedCkTypes;
    
    for (int row = 0; row < mRecordTypeTable->rowCount(); ++row) {
        auto* checkItem = mRecordTypeTable->item(row, 2);
        if (checkItem && checkItem->checkState() == Qt::Checked) {
            QString typeName = mRecordTypeTable->item(row, 0)->text();
            selectedTypes.append(typeName);
            
            // Map to CkId::Type
            if (typeName == "NPC_") selectedCkTypes.append(CkId::Type_Npc_);
            else if (typeName == "WEAP_") selectedCkTypes.append(CkId::Type_Weap_);
            else if (typeName == "ARMOR_") selectedCkTypes.append(CkId::Type_Armor_);
            else if (typeName == "SPEL_") selectedCkTypes.append(CkId::Type_Spel_);
            else if (typeName == "MGEF") selectedCkTypes.append(CkId::Type_Magic_);
            else if (typeName == "QUST_") selectedCkTypes.append(CkId::Type_Quest_);
            else if (typeName == "DIAL_") selectedCkTypes.append(CkId::Type_Dial_);
            else if (typeName == "INFO_") selectedCkTypes.append(CkId::Type_Info_);
            else if (typeName == "PACK_") selectedCkTypes.append(CkId::Type_Pack_);
            else if (typeName == "ALCH_") selectedCkTypes.append(CkId::Type_Alch_);
            else if (typeName == "INGR_") selectedCkTypes.append(CkId::Type_Ingr_);
            else if (typeName == "CONT_") selectedCkTypes.append(CkId::Type_Cont_);
            else if (typeName == "ENCH_") selectedCkTypes.append(CkId::Type_Ench_);
            else if (typeName == "BOOK_") selectedCkTypes.append(CkId::Type_Book_);
            else if (typeName == "MISC_") selectedCkTypes.append(CkId::Type_Misc_);
            else if (typeName == "ACTI_") selectedCkTypes.append(CkId::Type_Acti_);
            else if (typeName == "STAT_") selectedCkTypes.append(CkId::Type_Stat_);
            else if (typeName == "RACE_") selectedCkTypes.append(CkId::Type_Race_);
            else if (typeName == "CLASS_") selectedCkTypes.append(CkId::Type_Class_);
            else if (typeName == "FACT_") selectedCkTypes.append(CkId::Type_Fact_);
            else if (typeName == "PERK_") selectedCkTypes.append(CkId::Type_PerK_);
            else if (typeName == "CEL_") selectedCkTypes.append(CkId::Type_Cel_);
            else if (typeName == "WRLD_") selectedCkTypes.append(CkId::Type_WRLD_);
            else if (typeName == "LOCT_") selectedCkTypes.append(CkId::Type_LOCT_);
            else if (typeName == "SOUN_") selectedCkTypes.append(CkId::Type_Soun_);
            else if (typeName == "WTHR_") selectedCkTypes.append(CkId::Type_Wthr_);
            else if (typeName == "LTEX_") selectedCkTypes.append(CkId::Type_Ltex_);
        }
    }

    if (selectedTypes.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select at least one record type to export.");
        return;
    }

    QString outputDir = selectExportDir();
    if (outputDir.isEmpty()) {
        return;
    }

    // Build filter
    DataExporter::ExportFilter filter;
    filter.editorIdPattern = mEditorIdFilter->text();
    filter.formIdMin = static_cast<quint32>(mFormIdMin->value());
    filter.formIdMax = static_cast<quint32>(mFormIdMax->value());
    filter.onlyModified = mOnlyModified->isChecked();
    filter.onlyDeleted = mOnlyDeleted->isChecked();

    // Export based on format
    QString format = mRecordFormatCombo->currentText();
    QString outputPath = outputDir + "/export." + format.toLower();

    logMessage(QString("Exporting %1 record type(s) as %2...").arg(selectedTypes.size()).arg(format));
    mProgress->setLabelText(QString("Exporting records..."));
    mProgress->setValue(0);
    mProgress->show();

    DataExporter::ExportResult result;

    if (format == "JSON") {
        result = DataExporter::exportToJSON(*mData, selectedTypes, outputPath, filter);
    } else if (format == "CSV") {
        result = DataExporter::exportToCSV(*mData, selectedTypes, outputPath, filter);
    } else if (format == "XML") {
        result = DataExporter::exportToXML(*mData, selectedTypes, outputPath, filter);
    }

    mProgress->hide();

    if (result.error.isEmpty()) {
        logMessage(QString("Successfully exported %1 record(s)").arg(result.recordsExported));
        logMessage(QString("Output: %1 (%2 bytes)")
            .arg(outputPath)
            .arg(result.fileSize));
        
        for (auto it = result.recordsByType.begin(); it != result.recordsByType.end(); ++it) {
            logMessage(QString("  %1: %2 record(s)").arg(it.key()).arg(it.value()));
        }

        QMessageBox::information(this, "Export Complete",
            QString("Exported %1 record(s) to:\n%2")
                .arg(result.recordsExported)
                .arg(outputPath));
    } else {
        logMessage(QString("Export failed: %1").arg(result.error));
        QMessageBox::critical(this, "Export Failed", result.error);
    }
}

void BatchExportDialog::onConvertAssets()
{
    if (mAssetFileList->count() == 0) {
        QMessageBox::warning(this, "No Files", "Please add files to convert.");
        return;
    }

    QString outputDir = selectExportDir();
    if (outputDir.isEmpty()) {
        return;
    }

    int typeIdx = mAssetTypeCombo->currentIndex();
    QString targetFormat = mAssetTargetFormatCombo->currentText().toLower();

    QStringList inputFiles;
    for (int i = 0; i < mAssetFileList->count(); ++i) {
        inputFiles.append(mAssetFileList->item(i)->text());
    }

    logMessage(QString("Converting %1 file(s) to %2...").arg(inputFiles.size()).arg(targetFormat.toUpper()));
    mProgress->setLabelText(QString("Converting assets..."));
    mProgress->setValue(0);
    mProgress->show();

    AssetConverter::ConversionResult result;

    if (typeIdx == 0) { // NIF -> OBJ
        int converted = 0;
        int failed = 0;
        for (const auto& file : inputFiles) {
            QString outputPath = outputDir + "/" + QFileInfo(file).baseName() + ".obj";
            AssetConverter::ConversionResult r = AssetConverter::nifToObj(file, outputPath);
            if (r.success) {
                converted++;
                logMessage(QString("  %1 -> %2").arg(QFileInfo(file).fileName(), QFileInfo(outputPath).fileName()));
            } else {
                failed++;
                logMessage(QString("  FAILED: %1 - %2").arg(QFileInfo(file).fileName(), r.error));
            }
            mProgress->setValue(static_cast<int>((converted + failed) * 100.0 / inputFiles.size()));
        }
        result.success = true;
        result.filesConverted = converted;
        if (failed > 0) {
            result.error = QString("%1 file(s) failed").arg(failed);
        }
    } else if (typeIdx == 1) { // Textures
        result = AssetConverter::convertTextures(inputFiles, outputDir, targetFormat);
    } else { // Sounds
        result = AssetConverter::convertSounds(inputFiles, outputDir);
    }

    mProgress->hide();

    if (result.success) {
        logMessage(QString("Converted %1 file(s)").arg(result.filesConverted));
        QMessageBox::information(this, "Conversion Complete",
            QString("Successfully converted %1 file(s) to:\n%2")
                .arg(result.filesConverted)
                .arg(outputDir));
    } else {
        logMessage(QString("Conversion failed: %1").arg(result.error));
        QMessageBox::critical(this, "Conversion Failed", result.error);
    }
}

QString BatchExportDialog::selectExportDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Export Directory");
    if (dir.isEmpty()) {
        logMessage("Export cancelled - no directory selected");
    }
    return dir;
}

void BatchExportDialog::onUpdateProgress(int value, const QString& text)
{
    mProgress->setValue(value);
    if (!text.isEmpty()) {
        mProgress->setLabelText(text);
    }
}

void BatchExportDialog::onComplete()
{
    mProgress->hide();
}
