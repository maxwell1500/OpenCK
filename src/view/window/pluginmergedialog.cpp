#include "pluginmergedialog.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/ckid.hpp"
#include "logger.hpp"
#include "../../../libs/files/esm/esmreader.hpp"

#include <QMessageBox>
#include <QFileDialog>
#include <QCheckBox>
#include <QScrollArea>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QComboBox>

template<typename RecordT, typename SearchFunc>
static void scanConflictsFromESM(
    Data* data,
    const QString& fullPath,
    NAME esmTag,
    SearchFunc searchFunc,
    const QString& sourceFile,
    const QString& recordType,
    QVector<MergeConflict>& conflicts)
{
    try
    {
        ESMReader reader(fullPath, data->getPaths());
        reader.open();

        while (reader.isLeft())
        {
            NAME name = reader.readName();

            if (name == 'GRUP')
            {
                reader.skipGrupHeader();
                continue;
            }

            if (name != esmTag)
            {
                reader.skipRecord();
                continue;
            }

            try
            {
                RecordT record;
                record.load(reader, true);

                if (searchFunc(record.editorId) != NOT_FOUND)
                {
                    MergeConflict mc;
                    mc.recordType = recordType;
                    mc.editorId = record.editorId;
                    mc.sourceFile = sourceFile;
                    mc.resolution = ConflictResolution::AutoRename;
                    conflicts.append(mc);
                }
            }
            catch (const std::exception& e)
            {
                continue;
            }
        }
    }
    catch (const std::exception& e)
    {
        LOG_WARNING(QString("Plugin merge error: %1").arg(e.what()));
    }
}

template<typename RecordT, typename AddFunc, typename SearchFunc, typename RemoveFunc>
static int mergeRecordTypeFromESM(
    Data* data,
    const QString& fullPath,
    NAME esmTag,
    AddFunc addFunc,
    SearchFunc searchFunc,
    RemoveFunc removeFunc,
    quint32& nextFormId,
    const QMap<QString, ConflictResolution>& resolutions,
    QVector<MergeConflict>& newConflicts,
    const QString& sourceFile,
    const QString& recordType)
{
    int merged = 0;

    try
    {
        ESMReader reader(fullPath, data->getPaths());
        reader.open();

        while (reader.isLeft())
        {
            NAME name = reader.readName();

            if (name == 'GRUP')
            {
                reader.skipGrupHeader();
                continue;
            }

            if (name != esmTag)
            {
                reader.skipRecord();
                continue;
            }

            try
            {
                RecordT record;
                record.load(reader, true);

                QString origId = record.editorId;
                if (searchFunc(origId) != NOT_FOUND)
                {
                    ConflictResolution res = ConflictResolution::AutoRename;
                    QMap<QString, ConflictResolution>::const_iterator it = resolutions.find(origId);
                    if (it != resolutions.end())
                        res = it.value();

                    switch (res)
                    {
                        case ConflictResolution::KeepSource:
                        {
                            LOG_INFO(QString("Conflict for '%1': keeping source version").arg(origId));
                            removeFunc(origId);
                            record.formId = nextFormId++;
                            if (addFunc(record))
                                merged++;
                            break;
                        }
                        case ConflictResolution::KeepDestination:
                        {
                            LOG_INFO(QString("Conflict for '%1': keeping destination version").arg(origId));
                            break;
                        }
                        case ConflictResolution::AutoRename:
                        default:
                        {
                            int suffix = 1;
                            while (searchFunc(origId + QString("_merged%1").arg(suffix)) != NOT_FOUND)
                                suffix++;
                            QString newName = origId + QString("_merged%1").arg(suffix);
                            LOG_INFO(QString("Auto-resolved conflict for '%1': renamed to '%2'")
                                .arg(origId).arg(newName));
                            record.editorId = newName;
                            record.formId = nextFormId++;
                            if (addFunc(record))
                                merged++;
                            break;
                        }
                    }
                }
                else
                {
                    record.formId = nextFormId++;

                    if (addFunc(record))
                        merged++;
                }
            }
            catch (const std::exception& e)
            {
                LOG_ERROR(QString("Skipping record during merge: %1").arg(e.what()));
                continue;
            }
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(QString("Failed to open source file for merge: %1 - %2")
            .arg(fullPath).arg(QString::fromStdString(e.what())));
    }

    return merged;
}

PluginMergeDialog::PluginMergeDialog(Data* data, QWidget* parent)
    : QDialog(parent)
    , mData(data)
    , sourceList(nullptr)
    , destList(nullptr)
    , mergeButton(nullptr)
    , selectAllButton(nullptr)
    , deselectAllButton(nullptr)
    , progressBar(nullptr)
    , statusLabel(nullptr)
{
    LOG_DEBUG("PluginMergeDialog created");
    setWindowTitle("Plugin Merge");
    setMinimumSize(700, 550);
    setupUI();
}

PluginMergeDialog::~PluginMergeDialog()
{
    LOG_DEBUG("PluginMergeDialog destroyed");
}

void PluginMergeDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* infoLabel = new QLabel("Select source plugins and record types to merge into a destination plugin.", this);
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    mainLayout->addWidget(new QLabel("Source plugins:", this));
    sourceList = new QListWidget(this);
    sourceList->setSelectionMode(QAbstractItemView::MultiSelection);
    mainLayout->addWidget(sourceList);

    auto* buttonLayout = new QHBoxLayout();
    selectAllButton = new QPushButton("Select All", this);
    deselectAllButton = new QPushButton("Deselect All", this);
    buttonLayout->addWidget(selectAllButton);
    buttonLayout->addWidget(deselectAllButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    mainLayout->addWidget(new QLabel("Record types to merge:", this));
    recordTypeCheckboxes = new QWidget(this);
    auto* checkboxLayout = new QVBoxLayout(recordTypeCheckboxes);
    checkboxLayout->setContentsMargins(0, 0, 0, 0);

    checkboxLayout->addWidget(makeCheckbox("NPC_", true));
    checkboxLayout->addWidget(makeCheckbox("WEAP_", true));
    checkboxLayout->addWidget(makeCheckbox("ARMOR_", true));
    checkboxLayout->addWidget(makeCheckbox("SPEL_", true));
    checkboxLayout->addWidget(makeCheckbox("QUST_", true));
    checkboxLayout->addWidget(makeCheckbox("ALCH_", true));
    checkboxLayout->addWidget(makeCheckbox("INGR_", true));
    checkboxLayout->addWidget(makeCheckbox("BOOK_", true));
    checkboxLayout->addWidget(makeCheckbox("ENCH_", true));
    checkboxLayout->addWidget(makeCheckbox("CONT_", true));
    checkboxLayout->addWidget(makeCheckbox("MISC_", true));
    checkboxLayout->addWidget(makeCheckbox("ACTI_", true));
    checkboxLayout->addWidget(makeCheckbox("STAT_", true));
    checkboxLayout->addWidget(makeCheckbox("PACK_", true));
    checkboxLayout->addWidget(makeCheckbox("RACE_", true));
    checkboxLayout->addWidget(makeCheckbox("CLASS_", true));
    checkboxLayout->addWidget(makeCheckbox("FACT_", true));
    checkboxLayout->addWidget(makeCheckbox("PERK_", true));

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidget(recordTypeCheckboxes);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMaximumHeight(200);
    mainLayout->addWidget(scrollArea);

    mainLayout->addWidget(new QLabel("Destination plugin (first source will be used as base):", this));
    destList = new QListWidget(this);
    destList->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(destList);

    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    mainLayout->addWidget(progressBar);

    mainLayout->addWidget(new QLabel("Merge Preview:", this));
    mPreviewTable = new QTableWidget(this);
    mPreviewTable->setColumnCount(5);
    mPreviewTable->setHorizontalHeaderLabels({"Record Type", "Source Count", "Conflicts", "Auto-Resolved", "Manual Required"});
    mPreviewTable->horizontalHeader()->setStretchLastSection(true);
    mPreviewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mPreviewTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mPreviewTable->setMinimumHeight(120);
    mPreviewTable->setVisible(false);
    mainLayout->addWidget(mPreviewTable);

    mPreviewStats = new QLabel("", this);
    mPreviewStats->setVisible(false);
    mainLayout->addWidget(mPreviewStats);

    statusLabel = new QLabel("Ready", this);
    mainLayout->addWidget(statusLabel);

    auto* bottomLayout = new QHBoxLayout();
    previewButton = new QPushButton("Generate Preview", this);
    previewButton->setEnabled(false);
    mergeButton = new QPushButton("Merge", this);
    mergeButton->setEnabled(false);
    auto* cancelButton = new QPushButton("Cancel", this);
    bottomLayout->addStretch();
    bottomLayout->addWidget(previewButton);
    bottomLayout->addWidget(mergeButton);
    bottomLayout->addWidget(cancelButton);
    mainLayout->addLayout(bottomLayout);

    connect(mergeButton, &QPushButton::clicked, this, &PluginMergeDialog::onMerge);
    connect(previewButton, &QPushButton::clicked, this, &PluginMergeDialog::onGeneratePreview);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(selectAllButton, &QPushButton::clicked, this, &PluginMergeDialog::onSelectAll);
    connect(deselectAllButton, &QPushButton::clicked, this, &PluginMergeDialog::onDeselectAll);
    connect(sourceList, &QListWidget::itemSelectionChanged, this, [this]() {
        mergeButton->setEnabled(sourceList->selectedItems().count() >= 1);
        previewButton->setEnabled(sourceList->selectedItems().count() >= 1);
        destList->clear();
        QList<QListWidgetItem*> selected = sourceList->selectedItems();
        if (!selected.isEmpty())
        {
            destList->addItem(selected.first()->text());
        }
    });
}

QCheckBox* PluginMergeDialog::makeCheckbox(const QString& text, bool checked)
{
    QCheckBox* cb = new QCheckBox(text, recordTypeCheckboxes);
    cb->setChecked(checked);
    return cb;
}

void PluginMergeDialog::onMerge()
{
    if (!mData) return;

    QStringList files = mData->getContentFiles();
    if (files.isEmpty())
    {
        QMessageBox::warning(this, "Merge Failed", "No plugin files loaded.");
        return;
    }

    QList<QListWidgetItem*> selectedItems = sourceList->selectedItems();
    if (selectedItems.isEmpty())
    {
        QMessageBox::warning(this, "Merge Failed", "Select at least one source plugin.");
        return;
    }

    mSourceFiles.clear();
    for (QListWidgetItem* item : selectedItems)
    {
        mSourceFiles << item->text();
    }

    QString destFileName = selectedItems.first()->text();
    if (!destFileName.endsWith(".esp", Qt::CaseInsensitive) && !destFileName.endsWith(".esl", Qt::CaseInsensitive))
    {
        destFileName.chop(destFileName.length() - destFileName.lastIndexOf('.'));
        destFileName += "_merged.esp";
    }

    mSelectedTypes.clear();
    QWidget* parent = recordTypeCheckboxes;
    QList<QCheckBox*> checkboxes = parent->findChildren<QCheckBox*>();
    for (QCheckBox* child : checkboxes)
    {
        if (child && child->isChecked())
        {
            mSelectedTypes << child->text();
        }
    }

    progressBar->setVisible(true);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    statusLabel->setText("Scanning for conflicts...");
    mergeButton->setEnabled(false);

    collectConflicts();

    QMap<QString, ConflictResolution> resolutions;
    for (const MergeConflict& mc : mConflicts)
    {
        if (!resolutions.contains(mc.editorId))
            resolutions[mc.editorId] = mc.resolution;
    }

    bool hasManualRequired = false;
    for (const MergeConflict& mc : mConflicts)
    {
        if (mc.resolution == ConflictResolution::AutoRename)
        {
            hasManualRequired = true;
            break;
        }
    }

    if (hasManualRequired)
    {
        if (!showManualResolutionDialog())
        {
            progressBar->setVisible(false);
            statusLabel->setText("Merge cancelled.");
            mergeButton->setEnabled(true);
            return;
        }

        resolutions.clear();
        for (const MergeConflict& mc : mConflicts)
        {
            if (!resolutions.contains(mc.editorId))
                resolutions[mc.editorId] = mc.resolution;
        }
    }

    int totalSteps = mSourceFiles.size() * mSelectedTypes.size() + 5;
    int step = 0;

    progressBar->setValue(0);
    step++;
    progressBar->setValue(step * 100 / totalSteps);
    statusLabel->setText("Analyzing records...");

    int totalMerged = 0;
    int totalSkipped = 0;

    for (int si = 0; si < mSourceFiles.size(); si++)
    {
        QString sourceFile = mSourceFiles[si];

        if (sourceFile == destFileName)
            continue;

        for (int ti = 0; ti < mSelectedTypes.size(); ti++)
        {
            QString typeName = mSelectedTypes[ti];
            step++;
            progressBar->setValue(step * 100 / totalSteps);
            statusLabel->setText(QString("%1/%2 - Merging %3 from %4")
                .arg(ti + 1).arg(mSelectedTypes.size())
                .arg(typeName).arg(sourceFile));

            QVector<MergeConflict> newConflicts;
            bool success = mergeType(mData, typeName, sourceFile, resolutions, newConflicts);
            if (success)
            {
                totalMerged++;
            }
            else
            {
                totalSkipped++;
            }
        }
    }

    step++;
    progressBar->setValue(100);
    statusLabel->setText(QString("Merge complete: %1 record types merged, %2 skipped").arg(totalMerged).arg(totalSkipped));
    mergeButton->setEnabled(true);

    QMessageBox::information(this, "Merge Complete",
        QString("Merge finished:\n%1 record types merged\n%2 types skipped (already in destination or no data)")
            .arg(totalMerged).arg(totalSkipped));
}

bool PluginMergeDialog::mergeType(Data* data, const QString& typeName, const QString& sourceFile,
    const QMap<QString, ConflictResolution>& resolutions,
    QVector<MergeConflict>& newConflicts)
{
    if (!data) return false;

    FilePaths paths = data->getPaths();
    QString fullPath = paths.dataDir.path() + "/" + sourceFile;

    quint32 nextFormId = 0x80000000;

    if (typeName == "NPC_")
    {
        return mergeRecordTypeFromESM<NpcRecord>(
            data, fullPath, 'NPC_',
            [data](NpcRecord& r) { return data->addNpc(r); },
            [data](const QString& id) { return data->getNpcCollection().searchId(id); },
            [data](const QString& id) { data->removeRecord(CkId::Type_Npc_, id); },
            nextFormId, resolutions, newConflicts, sourceFile, typeName) > 0;
    }
    else if (typeName == "WEAP_")
    {
        return mergeRecordTypeFromESM<WeaponRecord>(
            data, fullPath, 'WEAP',
            [data](WeaponRecord& r) { return data->addWeapon(r); },
            [data](const QString& id) { return data->getWeaponCollection().searchId(id); },
            [data](const QString& id) { data->removeRecord(CkId::Type_Weap_, id); },
            nextFormId, resolutions, newConflicts, sourceFile, typeName) > 0;
    }
    else if (typeName == "ARMOR_")
    {
        return mergeRecordTypeFromESM<ArmorRecord>(
            data, fullPath, 'ARMO',
            [data](ArmorRecord& r) { return data->addArmor(r); },
            [data](const QString& id) { return data->getArmorCollection().searchId(id); },
            [data](const QString& id) { data->removeRecord(CkId::Type_Armor_, id); },
            nextFormId, resolutions, newConflicts, sourceFile, typeName) > 0;
    }
    else if (typeName == "SPEL_")
    {
        return mergeRecordTypeFromESM<SpellRecord>(
            data, fullPath, 'SPEL',
            [data](SpellRecord& r) { return data->addSpell(r); },
            [data](const QString& id) { return data->getSpellCollection().searchId(id); },
            [data](const QString& id) { data->removeRecord(CkId::Type_Spel_, id); },
            nextFormId, resolutions, newConflicts, sourceFile, typeName) > 0;
    }
    else if (typeName == "QUST_")
    {
        return mergeRecordTypeFromESM<QuestRecord>(
            data, fullPath, 'QUST',
            [data](QuestRecord& r) { return data->addQuest(r); },
            [data](const QString& id) { return data->getQuestCollection().searchId(id); },
            [data](const QString& id) { data->removeRecord(CkId::Type_Quest_, id); },
            nextFormId, resolutions, newConflicts, sourceFile, typeName) > 0;
    }
    else if (typeName == "ALCH_")
    {
        return mergeRecordTypeFromESM<AlchRecord>(
            data, fullPath, 'ALCH',
            [data](AlchRecord& r) { return data->addAlch(r); },
            [data](const QString& id) { return data->getAlchCollection().searchId(id); },
            [data](const QString& id) { data->removeRecord(CkId::Type_Alch_, id); },
            nextFormId, resolutions, newConflicts, sourceFile, typeName) > 0;
    }
    else if (typeName == "INGR_")
    {
        return mergeRecordTypeFromESM<IngrRecord>(
            data, fullPath, 'INGR',
            [data](IngrRecord& r) { return data->addIngr(r); },
            [data](const QString& id) { return data->getIngrCollection().searchId(id); },
            [data](const QString& id) { data->removeRecord(CkId::Type_Ingr_, id); },
            nextFormId, resolutions, newConflicts, sourceFile, typeName) > 0;
    }
    else if (typeName == "BOOK_")
    {
        return mergeRecordTypeFromESM<BookRecord>(
            data, fullPath, 'BOOK',
            [data](BookRecord& r) { return data->addBook(r); },
            [data](const QString& id) { return data->getBookCollection().searchId(id); },
            [data](const QString& id) { data->removeRecord(CkId::Type_Book_, id); },
            nextFormId, resolutions, newConflicts, sourceFile, typeName) > 0;
    }
    else if (typeName == "ENCH_")
    {
        return mergeRecordTypeFromESM<EnchRecord>(
            data, fullPath, 'ENCH',
            [data](EnchRecord& r) { return data->addEnch(r); },
            [data](const QString& id) { return data->getEnchCollection().searchId(id); },
            [data](const QString& id) { data->removeRecord(CkId::Type_Ench_, id); },
            nextFormId, resolutions, newConflicts, sourceFile, typeName) > 0;
    }
    else if (typeName == "CONT_")
    {
        return mergeRecordTypeFromESM<ContRecord>(
            data, fullPath, 'CONT',
            [data](ContRecord& r) { return data->addCont(r); },
            [data](const QString& id) { return data->getContCollection().searchId(id); },
            [data](const QString& id) { data->removeRecord(CkId::Type_Cont_, id); },
            nextFormId, resolutions, newConflicts, sourceFile, typeName) > 0;
    }
    else if (typeName == "MISC_")
    {
        return mergeRecordTypeFromESM<MiscRecord>(
            data, fullPath, 'MISC',
            [data](MiscRecord& r) { return data->addMisc(r); },
            [data](const QString& id) { return data->getMiscCollection().searchId(id); },
            [data](const QString& id) { data->removeRecord(CkId::Type_Misc_, id); },
            nextFormId, resolutions, newConflicts, sourceFile, typeName) > 0;
    }
    else if (typeName == "ACTI_")
    {
        return mergeRecordTypeFromESM<ActiRecord>(
            data, fullPath, 'ACTI',
            [data](ActiRecord& r) { return data->addActi(r); },
            [data](const QString& id) { return data->getActiCollection().searchId(id); },
            [data](const QString& id) { data->removeRecord(CkId::Type_Acti_, id); },
            nextFormId, resolutions, newConflicts, sourceFile, typeName) > 0;
    }
    else if (typeName == "STAT_")
    {
        return mergeRecordTypeFromESM<StatRecord>(
            data, fullPath, 'STAT',
            [data](StatRecord& r) { return data->addStat(r); },
            [data](const QString& id) { return data->getStatCollection().searchId(id); },
            [data](const QString& id) { data->removeRecord(CkId::Type_Stat_, id); },
            nextFormId, resolutions, newConflicts, sourceFile, typeName) > 0;
    }
    else if (typeName == "PACK_")
    {
        return mergeRecordTypeFromESM<PackageRecord>(
            data, fullPath, 'PACK',
            [data](PackageRecord& r) { return data->addPack(r); },
            [data](const QString& id) { return data->getPackCollection().searchId(id); },
            [data](const QString& id) { data->removeRecord(CkId::Type_Pack_, id); },
            nextFormId, resolutions, newConflicts, sourceFile, typeName) > 0;
    }
    else if (typeName == "RACE_")
    {
        return mergeRecordTypeFromESM<RaceRecord>(
            data, fullPath, 'RACE',
            [data](RaceRecord& r) { return data->addRace(r); },
            [data](const QString& id) { return data->getRaceCollection().searchId(id); },
            [data](const QString& id) { data->removeRecord(CkId::Type_Race_, id); },
            nextFormId, resolutions, newConflicts, sourceFile, typeName) > 0;
    }
    else if (typeName == "CLASS_")
    {
        return mergeRecordTypeFromESM<ClassRecord>(
            data, fullPath, 'CLAS',
            [data](ClassRecord& r) { return data->addClass(r); },
            [data](const QString& id) { return data->getClassCollection().searchId(id); },
            [data](const QString& id) { data->removeRecord(CkId::Type_Class_, id); },
            nextFormId, resolutions, newConflicts, sourceFile, typeName) > 0;
    }
    else if (typeName == "FACT_")
    {
        return mergeRecordTypeFromESM<FactRecord>(
            data, fullPath, 'FACT',
            [data](FactRecord& r) { return data->addFact(r); },
            [data](const QString& id) { return data->getFactCollection().searchId(id); },
            [data](const QString& id) { data->removeRecord(CkId::Type_Fact_, id); },
            nextFormId, resolutions, newConflicts, sourceFile, typeName) > 0;
    }
    else if (typeName == "PERK_")
    {
        return mergeRecordTypeFromESM<PerkRecord>(
            data, fullPath, 'PERK',
            [data](PerkRecord& r) { return data->addPerk(r); },
            [data](const QString& id) { return data->getPerkCollection().searchId(id); },
            [data](const QString& id) { data->removeRecord(CkId::Type_PerK_, id); },
            nextFormId, resolutions, newConflicts, sourceFile, typeName) > 0;
    }

    LOG_WARNING(QString("Merge type '%1' is not supported").arg(typeName));
    return false;
}

void PluginMergeDialog::onSelectAll()
{
    for (int i = 0; i < sourceList->count(); i++)
    {
        sourceList->item(i)->setSelected(true);
    }
}

void PluginMergeDialog::onDeselectAll()
{
    for (int i = 0; i < sourceList->count(); i++)
    {
        sourceList->item(i)->setSelected(false);
    }
}

void PluginMergeDialog::collectConflicts()
{
    mConflicts.clear();

    FilePaths paths = mData->getPaths();

    for (const QString& sourceFile : mSourceFiles)
    {
        QString fullPath = paths.dataDir.path() + "/" + sourceFile;

        for (const QString& typeName : mSelectedTypes)
        {
            if (typeName == "NPC_")
            {
                scanConflictsFromESM<NpcRecord>(
                    mData, fullPath, 'NPC_',
                    [this](const QString& id) { return mData->getNpcCollection().searchId(id); },
                    sourceFile, typeName, mConflicts);
            }
            else if (typeName == "WEAP_")
            {
                scanConflictsFromESM<WeaponRecord>(
                    mData, fullPath, 'WEAP',
                    [this](const QString& id) { return mData->getWeaponCollection().searchId(id); },
                    sourceFile, typeName, mConflicts);
            }
            else if (typeName == "ARMOR_")
            {
                scanConflictsFromESM<ArmorRecord>(
                    mData, fullPath, 'ARMO',
                    [this](const QString& id) { return mData->getArmorCollection().searchId(id); },
                    sourceFile, typeName, mConflicts);
            }
            else if (typeName == "SPEL_")
            {
                scanConflictsFromESM<SpellRecord>(
                    mData, fullPath, 'SPEL',
                    [this](const QString& id) { return mData->getSpellCollection().searchId(id); },
                    sourceFile, typeName, mConflicts);
            }
            else if (typeName == "QUST_")
            {
                scanConflictsFromESM<QuestRecord>(
                    mData, fullPath, 'QUST',
                    [this](const QString& id) { return mData->getQuestCollection().searchId(id); },
                    sourceFile, typeName, mConflicts);
            }
            else if (typeName == "ALCH_")
            {
                scanConflictsFromESM<AlchRecord>(
                    mData, fullPath, 'ALCH',
                    [this](const QString& id) { return mData->getAlchCollection().searchId(id); },
                    sourceFile, typeName, mConflicts);
            }
            else if (typeName == "INGR_")
            {
                scanConflictsFromESM<IngrRecord>(
                    mData, fullPath, 'INGR',
                    [this](const QString& id) { return mData->getIngrCollection().searchId(id); },
                    sourceFile, typeName, mConflicts);
            }
            else if (typeName == "BOOK_")
            {
                scanConflictsFromESM<BookRecord>(
                    mData, fullPath, 'BOOK',
                    [this](const QString& id) { return mData->getBookCollection().searchId(id); },
                    sourceFile, typeName, mConflicts);
            }
            else if (typeName == "ENCH_")
            {
                scanConflictsFromESM<EnchRecord>(
                    mData, fullPath, 'ENCH',
                    [this](const QString& id) { return mData->getEnchCollection().searchId(id); },
                    sourceFile, typeName, mConflicts);
            }
            else if (typeName == "CONT_")
            {
                scanConflictsFromESM<ContRecord>(
                    mData, fullPath, 'CONT',
                    [this](const QString& id) { return mData->getContCollection().searchId(id); },
                    sourceFile, typeName, mConflicts);
            }
            else if (typeName == "MISC_")
            {
                scanConflictsFromESM<MiscRecord>(
                    mData, fullPath, 'MISC',
                    [this](const QString& id) { return mData->getMiscCollection().searchId(id); },
                    sourceFile, typeName, mConflicts);
            }
            else if (typeName == "ACTI_")
            {
                scanConflictsFromESM<ActiRecord>(
                    mData, fullPath, 'ACTI',
                    [this](const QString& id) { return mData->getActiCollection().searchId(id); },
                    sourceFile, typeName, mConflicts);
            }
            else if (typeName == "STAT_")
            {
                scanConflictsFromESM<StatRecord>(
                    mData, fullPath, 'STAT',
                    [this](const QString& id) { return mData->getStatCollection().searchId(id); },
                    sourceFile, typeName, mConflicts);
            }
            else if (typeName == "PACK_")
            {
                scanConflictsFromESM<PackageRecord>(
                    mData, fullPath, 'PACK',
                    [this](const QString& id) { return mData->getPackCollection().searchId(id); },
                    sourceFile, typeName, mConflicts);
            }
            else if (typeName == "RACE_")
            {
                scanConflictsFromESM<RaceRecord>(
                    mData, fullPath, 'RACE',
                    [this](const QString& id) { return mData->getRaceCollection().searchId(id); },
                    sourceFile, typeName, mConflicts);
            }
            else if (typeName == "CLASS_")
            {
                scanConflictsFromESM<ClassRecord>(
                    mData, fullPath, 'CLAS',
                    [this](const QString& id) { return mData->getClassCollection().searchId(id); },
                    sourceFile, typeName, mConflicts);
            }
            else if (typeName == "FACT_")
            {
                scanConflictsFromESM<FactRecord>(
                    mData, fullPath, 'FACT',
                    [this](const QString& id) { return mData->getFactCollection().searchId(id); },
                    sourceFile, typeName, mConflicts);
            }
            else if (typeName == "PERK_")
            {
                scanConflictsFromESM<PerkRecord>(
                    mData, fullPath, 'PERK',
                    [this](const QString& id) { return mData->getPerkCollection().searchId(id); },
                    sourceFile, typeName, mConflicts);
            }
        }
    }
}

bool PluginMergeDialog::showManualResolutionDialog()
{
    if (mConflicts.isEmpty())
        return true;

    QDialog dialog(this);
    dialog.setWindowTitle("Resolve Merge Conflicts");
    dialog.setMinimumSize(750, 450);

    auto* layout = new QVBoxLayout(&dialog);

    auto* label = new QLabel(
        QString("%1 conflicts found. Choose how to resolve each:").arg(mConflicts.size()), &dialog);
    layout->addWidget(label);

    auto* table = new QTableWidget(&dialog);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"Record Type", "Editor ID", "Source Plugin", "Resolution"});
    table->setRowCount(mConflicts.size());
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);

    QStringList resolutionLabels = {"Auto-Rename", "Keep Source", "Keep Destination"};

    for (int i = 0; i < mConflicts.size(); i++)
    {
        const MergeConflict& mc = mConflicts[i];
        table->setItem(i, 0, new QTableWidgetItem(mc.recordType));
        table->setItem(i, 1, new QTableWidgetItem(mc.editorId));
        table->setItem(i, 2, new QTableWidgetItem(mc.sourceFile));

        auto* combo = new QComboBox(&dialog);
        combo->addItems(resolutionLabels);
        table->setCellWidget(i, 3, combo);
    }

    layout->addWidget(table);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return false;

    for (int i = 0; i < mConflicts.size(); i++)
    {
        auto* combo = qobject_cast<QComboBox*>(table->cellWidget(i, 3));
        if (combo)
        {
            switch (combo->currentIndex())
            {
                case 0: mConflicts[i].resolution = ConflictResolution::AutoRename; break;
                case 1: mConflicts[i].resolution = ConflictResolution::KeepSource; break;
                case 2: mConflicts[i].resolution = ConflictResolution::KeepDestination; break;
            }
        }
    }

    return true;
}

void PluginMergeDialog::onGeneratePreview()
{
    if (!mData) return;

    QList<QListWidgetItem*> selectedItems = sourceList->selectedItems();
    if (selectedItems.isEmpty())
    {
        QMessageBox::warning(this, "Preview Failed", "Select at least one source plugin.");
        return;
    }

    QStringList sourceFiles;
    for (QListWidgetItem* item : selectedItems)
    {
        sourceFiles << item->text();
    }

    QList<QString> selectedTypes;
    QWidget* parent = recordTypeCheckboxes;
    QList<QCheckBox*> checkboxes = parent->findChildren<QCheckBox*>();
    for (QCheckBox* child : checkboxes)
    {
        if (child && child->isChecked())
        {
            selectedTypes << child->text();
        }
    }

    if (selectedTypes.isEmpty())
    {
        QMessageBox::warning(this, "Preview Failed", "Select at least one record type.");
        return;
    }

    mPreviewTable->setRowCount(0);
    mPreviewTable->setVisible(true);
    mPreviewStats->setVisible(true);

    int totalRecords = 0;
    int totalConflicts = 0;
    int totalAutoResolved = 0;

    auto countConflicts = [&](const QStringList& ids) -> QPair<int, int>
    {
        QMap<QString, int> idCounts;
        for (const QString& id : ids)
            idCounts[id]++;
        int conflicts = 0;
        int autoResolved = 0;
        for (auto it = idCounts.begin(); it != idCounts.end(); ++it)
        {
            if (it.value() > 1)
            {
                conflicts++;
            }
        }
        return qMakePair(conflicts, autoResolved);
    };

    for (int ti = 0; ti < selectedTypes.size(); ti++)
    {
        QString typeName = selectedTypes[ti];
        int sourceCount = 0;
        int conflicts = 0;
        int autoResolved = 0;

        if (typeName == "NPC_")
        {
            sourceCount = mData->getNpcCollection().size();
            QStringList ids = mData->getNpcCollection().getIds(false);
            auto result = countConflicts(ids);
            conflicts = result.first;
            autoResolved = result.second;
        }
        else if (typeName == "WEAP_")
        {
            sourceCount = mData->getWeaponCollection().size();
            QStringList ids = mData->getWeaponCollection().getIds(false);
            auto result = countConflicts(ids);
            conflicts = result.first;
            autoResolved = result.second;
        }
        else if (typeName == "ARMOR_")
        {
            sourceCount = mData->getArmorCollection().size();
            QStringList ids = mData->getArmorCollection().getIds(false);
            auto result = countConflicts(ids);
            conflicts = result.first;
            autoResolved = result.second;
        }
        else if (typeName == "SPEL_")
        {
            sourceCount = mData->getSpellCollection().size();
            QStringList ids = mData->getSpellCollection().getIds(false);
            auto result = countConflicts(ids);
            conflicts = result.first;
            autoResolved = result.second;
        }
        else if (typeName == "QUST_")
        {
            sourceCount = mData->getQuestCollection().size();
            QStringList ids = mData->getQuestCollection().getIds(false);
            auto result = countConflicts(ids);
            conflicts = result.first;
            autoResolved = result.second;
        }
        else if (typeName == "ALCH_")
        {
            sourceCount = mData->getAlchCollection().size();
            QStringList ids = mData->getAlchCollection().getIds(false);
            auto result = countConflicts(ids);
            conflicts = result.first;
            autoResolved = result.second;
        }
        else if (typeName == "INGR_")
        {
            sourceCount = mData->getIngrCollection().size();
            QStringList ids = mData->getIngrCollection().getIds(false);
            auto result = countConflicts(ids);
            conflicts = result.first;
            autoResolved = result.second;
        }
        else if (typeName == "BOOK_")
        {
            sourceCount = mData->getBookCollection().size();
            QStringList ids = mData->getBookCollection().getIds(false);
            auto result = countConflicts(ids);
            conflicts = result.first;
            autoResolved = result.second;
        }
        else if (typeName == "ENCH_")
        {
            sourceCount = mData->getEnchCollection().size();
            QStringList ids = mData->getEnchCollection().getIds(false);
            auto result = countConflicts(ids);
            conflicts = result.first;
            autoResolved = result.second;
        }
        else if (typeName == "CONT_")
        {
            sourceCount = mData->getContCollection().size();
            QStringList ids = mData->getContCollection().getIds(false);
            auto result = countConflicts(ids);
            conflicts = result.first;
            autoResolved = result.second;
        }
        else if (typeName == "MISC_")
        {
            sourceCount = mData->getMiscCollection().size();
            QStringList ids = mData->getMiscCollection().getIds(false);
            auto result = countConflicts(ids);
            conflicts = result.first;
            autoResolved = result.second;
        }
        else if (typeName == "ACTI_")
        {
            sourceCount = mData->getActiCollection().size();
            QStringList ids = mData->getActiCollection().getIds(false);
            auto result = countConflicts(ids);
            conflicts = result.first;
            autoResolved = result.second;
        }
        else if (typeName == "STAT_")
        {
            sourceCount = mData->getStatCollection().size();
            QStringList ids = mData->getStatCollection().getIds(false);
            auto result = countConflicts(ids);
            conflicts = result.first;
            autoResolved = result.second;
        }
        else if (typeName == "PACK_")
        {
            sourceCount = mData->getPackCollection().size();
            QStringList ids = mData->getPackCollection().getIds(false);
            auto result = countConflicts(ids);
            conflicts = result.first;
            autoResolved = result.second;
        }
        else if (typeName == "RACE_")
        {
            sourceCount = mData->getRaceCollection().size();
            QStringList ids = mData->getRaceCollection().getIds(false);
            auto result = countConflicts(ids);
            conflicts = result.first;
            autoResolved = result.second;
        }
        else if (typeName == "CLASS_")
        {
            sourceCount = mData->getClassCollection().size();
            QStringList ids = mData->getClassCollection().getIds(false);
            auto result = countConflicts(ids);
            conflicts = result.first;
            autoResolved = result.second;
        }
        else if (typeName == "FACT_")
        {
            sourceCount = mData->getFactCollection().size();
            QStringList ids = mData->getFactCollection().getIds(false);
            auto result = countConflicts(ids);
            conflicts = result.first;
            autoResolved = result.second;
        }
        else if (typeName == "PERK_")
        {
            sourceCount = mData->getPerkCollection().size();
            QStringList ids = mData->getPerkCollection().getIds(false);
            auto result = countConflicts(ids);
            conflicts = result.first;
            autoResolved = result.second;
        }

        totalRecords += sourceCount;
        totalConflicts += conflicts;
        totalAutoResolved += autoResolved;

        int row = mPreviewTable->rowCount();
        mPreviewTable->insertRow(row);
        mPreviewTable->setItem(row, 0, new QTableWidgetItem(typeName));
        mPreviewTable->setItem(row, 1, new QTableWidgetItem(QString::number(sourceCount)));
        mPreviewTable->setItem(row, 2, new QTableWidgetItem(QString::number(conflicts)));
        mPreviewTable->setItem(row, 3, new QTableWidgetItem(QString::number(autoResolved)));
        mPreviewTable->setItem(row, 4, new QTableWidgetItem(QString::number(conflicts - autoResolved)));
    }

    mPreviewStats->setText(QString("Total: %1 records | Conflicts: %2 | Auto-Resolved: %3 | Manual Required: %4")
        .arg(totalRecords)
        .arg(totalConflicts)
        .arg(totalAutoResolved)
        .arg(totalConflicts - totalAutoResolved));

    statusLabel->setText("Preview generated.");
}
