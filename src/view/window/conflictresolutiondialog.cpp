#include "conflictresolutiondialog.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/ckid.hpp"
#include "logger.hpp"

#include "../../../libs/files/esm/npcrecord.hpp"
#include "../../../libs/files/esm/weaprecord.hpp"
#include "../../../libs/files/esm/armorrecord.hpp"
#include "../../../libs/files/esm/Spellrecord.hpp"
#include "../../../libs/files/esm/Magicrecord.hpp"
#include "../../../libs/files/esm/Questrecord.hpp"
#include "../../../libs/files/esm/Dialrecord.hpp"
#include "../../../libs/files/esm/Inforecord.hpp"
#include "../../../libs/files/esm/esmwriter.hpp"
#include "../../../libs/files/esm/gmst.hpp"
#include "../../../libs/files/esm/glob.hpp"
#include "../../../libs/files/esm/lcrt.hpp"
#include "../../../libs/files/esm/Packagerecord.hpp"
#include "../../../libs/files/esm/treerecord.hpp"
#include "../../../libs/files/esm/alchrecord.hpp"
#include "../../../libs/files/esm/ingrrecord.hpp"
#include "../../../libs/files/esm/contrecord.hpp"
#include "../../../libs/files/esm/enchrecord.hpp"
#include "../../../libs/files/esm/bookrecord.hpp"
#include "../../../libs/files/esm/miscrecord.hpp"
#include "../../../libs/files/esm/actirecord.hpp"
#include "../../../libs/files/esm/statrecord.hpp"
#include "../../../libs/files/esm/racerecord.hpp"
#include "../../../libs/files/esm/classrecord.hpp"
#include "../../../libs/files/esm/factrecord.hpp"
#include "../../../libs/files/esm/perkrecord.hpp"
#include "../../../libs/files/esm/cellrecord.hpp"
#include "../../../libs/files/esm/worldspacerecord.hpp"
#include "../../../libs/files/esm/locationrecord.hpp"
#include "../../../libs/files/esm/refrecord.hpp"
#include "../../../libs/files/esm/materialrecord.hpp"

#include <QMessageBox>
#include <QFileDialog>

ConflictResolutionDialog::ConflictResolutionDialog(Data* data, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mConflictList(nullptr),
      mDetailA(nullptr),
      mDetailB(nullptr),
      mKeepAButton(nullptr),
      mKeepBButton(nullptr),
      mRemoveButton(nullptr),
      mSaveButton(nullptr),
      mStatusLabel(nullptr)
{
    LOG_INFO("ConflictResolutionDialog created");
    setupUI();
    loadConflicts();
}

ConflictResolutionDialog::~ConflictResolutionDialog()
{
}

void ConflictResolutionDialog::setupUI()
{
    setWindowTitle("Conflict Resolution");
    setMinimumSize(1000, 700);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    mStatusLabel = new QLabel("Scanning for conflicts...");
    mainLayout->addWidget(mStatusLabel);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    mConflictList = new QListWidget();
    mConflictList->setAlternatingRowColors(true);
    splitter->addWidget(mConflictList);

    auto* detailSplitter = new QSplitter(Qt::Vertical, this);

    mDetailA = new QListWidget();
    mDetailA->setAlternatingRowColors(true);
    mDetailA->setMaximumHeight(300);
    detailSplitter->addWidget(mDetailA);

    mDetailB = new QListWidget();
    mDetailB->setAlternatingRowColors(true);
    mDetailB->setMaximumHeight(300);
    detailSplitter->addWidget(mDetailB);

    splitter->addWidget(detailSplitter);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    mainLayout->addWidget(splitter, 1);

    auto* buttonLayout = new QHBoxLayout();
    mKeepAButton = new QPushButton("Keep Plugin A");
    mKeepAButton->setEnabled(false);
    buttonLayout->addWidget(mKeepAButton);

    mKeepBButton = new QPushButton("Keep Plugin B");
    mKeepBButton->setEnabled(false);
    buttonLayout->addWidget(mKeepBButton);

    mRemoveButton = new QPushButton("Remove Both");
    mRemoveButton->setEnabled(false);
    buttonLayout->addWidget(mRemoveButton);

    buttonLayout->addStretch();

    mSaveButton = new QPushButton("Save Changes");
    mSaveButton->setEnabled(false);
    buttonLayout->addWidget(mSaveButton);

    mainLayout->addLayout(buttonLayout);

    connect(mConflictList, &QListWidget::currentRowChanged, this, &ConflictResolutionDialog::onConflictSelected);
    connect(mKeepAButton, &QPushButton::clicked, this, &ConflictResolutionDialog::onKeepA);
    connect(mKeepBButton, &QPushButton::clicked, this, &ConflictResolutionDialog::onKeepB);
    connect(mRemoveButton, &QPushButton::clicked, this, &ConflictResolutionDialog::onRemoveBoth);
    connect(mSaveButton, &QPushButton::clicked, this, &ConflictResolutionDialog::onSaveChanges);
}

void ConflictResolutionDialog::loadConflicts()
{
    mConflicts = mData->detectConflicts();
    mConflictList->clear();

    for (const auto& conflict : mConflicts) {
        QString text = QString("[%1] %2 - %3 vs %4")
            .arg(CkId(conflict.type).getTypeName())
            .arg(conflict.editorId)
            .arg(conflict.pluginNameA)
            .arg(conflict.pluginNameB);
        mConflictList->addItem(text);
    }

    if (mConflicts.isEmpty()) {
        mStatusLabel->setText("No conflicts detected.");
    } else {
        mStatusLabel->setText(QString("Found %1 conflict(s)").arg(mConflicts.size()));
    }

    LOG_INFO(QString("Loaded %1 conflicts").arg(mConflicts.size()));
}

void ConflictResolutionDialog::onConflictSelected(int row)
{
    if (row < 0 || row >= mConflicts.size()) {
        mKeepAButton->setEnabled(false);
        mKeepBButton->setEnabled(false);
        mRemoveButton->setEnabled(false);
        mSaveButton->setEnabled(false);
        return;
    }

    const Data::ConflictInfo& conflict = mConflicts[row];
    showConflictDetails(conflict);

    mKeepAButton->setEnabled(true);
    mKeepBButton->setEnabled(true);
    mRemoveButton->setEnabled(true);
    mSaveButton->setEnabled(true);
}

void ConflictResolutionDialog::showConflictDetails(const Data::ConflictInfo& conflict)
{
    mDetailA->clear();
    mDetailB->clear();

    QString summaryA = getRecordSummary(conflict, conflict.pluginIndexA);
    QString summaryB = getRecordSummary(conflict, conflict.pluginIndexB);

    mDetailA->addItem(QString("Plugin: %1").arg(conflict.pluginNameA));
    mDetailA->addItem(QString("FormID: 0x%1").arg(conflict.editorId));
    mDetailA->addItem(summaryA);

    mDetailB->addItem(QString("Plugin: %1").arg(conflict.pluginNameB));
    mDetailB->addItem(QString("FormID: 0x%1").arg(conflict.editorId));
    mDetailB->addItem(summaryB);
}

QString ConflictResolutionDialog::getRecordSummary(const Data::ConflictInfo& conflict, int pluginIndex)
{
    QString pluginName = (pluginIndex == conflict.pluginIndexA) ? conflict.pluginNameA : conflict.pluginNameB;
    QString editorId = conflict.editorId;
    quint32 formId = 0;

    // Build summary based on type
    QString summary;
    switch (conflict.type) {
    case CkId::Type_Npc_: {
        const auto& npcCol = mData->getNpcCollection();
        int npcIdx = npcCol.searchId(editorId);
        if (npcIdx >= 0) {
            const NpcRecord& npc = npcCol.getRecord(npcIdx).get();
            formId = npc.formId;
            summary = QString("Name: %1 | Level: %2")
                .arg(npc.fullName).arg(npc.level);
        }
        break;
    }
    case CkId::Type_Weap_: {
        const auto& weapCol = mData->getWeaponCollection();
        int weapIdx = weapCol.searchId(editorId);
        if (weapIdx >= 0) {
            const WeaponRecord& weap = weapCol.getRecord(weapIdx).get();
            formId = weap.formId;
            summary = QString("Type: %1 | Damage: %2")
                .arg(weap.weaponType).arg(weap.damage);
        }
        break;
    }
    case CkId::Type_Armor_: {
        const auto& armorCol = mData->getArmorCollection();
        int armorIdx = armorCol.searchId(editorId);
        if (armorIdx >= 0) {
            const ArmorRecord& armor = armorCol.getRecord(armorIdx).get();
            formId = armor.formId;
            summary = QString("Armor: %1 | Weight: %2 | Value: %3")
                .arg(armor.armorRating).arg(armor.weight).arg(armor.value);
        }
        break;
    }
    case CkId::Type_Spel_: {
        const auto& spelCol = mData->getSpellCollection();
        int spelIdx = spelCol.searchId(editorId);
        if (spelIdx >= 0) {
            const SpellRecord& spell = spelCol.getRecord(spelIdx).get();
            formId = spell.formId;
            summary = QString("Cost: %1 | Effects: %2")
                .arg(spell.cost).arg(spell.effects.size());
        }
        break;
    }
    case CkId::Type_Magic_: {
        const auto& magicCol = mData->getMagicCollection();
        int magicIdx = magicCol.searchId(editorId);
        if (magicIdx >= 0) {
            const MagicRecord& magic = magicCol.getRecord(magicIdx).get();
            formId = magic.formId;
            summary = QString("Schools: %1 | Effects: %2")
                .arg(magic.schools).arg(magic.effects.size());
        }
        break;
    }
    case CkId::Type_Quest_: {
        const auto& questCol = mData->getQuestCollection();
        int questIdx = questCol.searchId(editorId);
        if (questIdx >= 0) {
            const QuestRecord& quest = questCol.getRecord(questIdx).get();
            formId = quest.formId;
            summary = QString("Name: %1 | Stages: %2 | Objectives: %3")
                .arg(quest.questName).arg(quest.stageIds.size()).arg(quest.objectiveIds.size());
        }
        break;
    }
    case CkId::Type_Dial_: {
        const auto& dialCol = mData->getDialCollection();
        int dialIdx = dialCol.searchId(editorId);
        if (dialIdx >= 0) {
            const DialRecord& dial = dialCol.getRecord(dialIdx).get();
            formId = dial.formId;
            summary = QString("Topic: %1 | Responses: %2")
                .arg(dial.topicName).arg(dial.responseIds.size());
        }
        break;
    }
    case CkId::Type_Info_: {
        const auto& infoCol = mData->getInfoCollection();
        int infoIdx = infoCol.searchId(editorId);
        if (infoIdx >= 0) {
            const InfoRecord& info = infoCol.getRecord(infoIdx).get();
            formId = info.formId;
            summary = QString("Response: %1... | Conditions: %2")
                .arg(info.responseText.left(50)).arg(info.conditionIds.size());
        }
        break;
    }
    default:
        summary = QString("FormID: 0x%1").arg(formId, 8, 16, QChar('0')).toUpper();
        break;
    }

    if (formId == 0) {
        return QString("Record from %1").arg(pluginName);
    }

    return QString("%1 | FormID: 0x%2 | %3")
        .arg(pluginName)
        .arg(formId, 8, 16, QChar('0')).toUpper()
        .arg(summary);
}

void ConflictResolutionDialog::onKeepA()
{
    int row = mConflictList->currentRow();
    if (row < 0 || row >= mConflicts.size())
        return;

    const Data::ConflictInfo& conflict = mConflicts[row];

    QString editorId = conflict.editorId;
    int pluginIndexB = conflict.pluginIndexB;

    // Get the appropriate collection and remove the record from plugin B
    bool removed = removeRecordByPlugin(conflict.type, editorId, pluginIndexB);

    if (removed) {
        LOG_INFO(QString("Kept Plugin A version of '%1', removed Plugin B").arg(editorId));
        mConflicts.removeAt(row);
        loadConflicts();
        QMessageBox::information(this, "Resolved",
            QString("Kept version from %1 for '%2'\nRemoved conflicting version from %3")
                .arg(conflict.pluginNameA).arg(editorId).arg(conflict.pluginNameB));
    } else {
        QMessageBox::critical(this, "Error",
            QString("Failed to resolve conflict for '%1'").arg(editorId));
    }
}

void ConflictResolutionDialog::onKeepB()
{
    int row = mConflictList->currentRow();
    if (row < 0 || row >= mConflicts.size())
        return;

    const Data::ConflictInfo& conflict = mConflicts[row];

    QString editorId = conflict.editorId;
    int pluginIndexA = conflict.pluginIndexA;

    // Get the appropriate collection and remove the record from plugin A
    bool removed = removeRecordByPlugin(conflict.type, editorId, pluginIndexA);

    if (removed) {
        LOG_INFO(QString("Kept Plugin B version of '%1', removed Plugin A").arg(editorId));
        mConflicts.removeAt(row);
        loadConflicts();
        QMessageBox::information(this, "Resolved",
            QString("Kept version from %1 for '%2'\nRemoved conflicting version from %3")
                .arg(conflict.pluginNameB).arg(editorId).arg(conflict.pluginNameA));
    } else {
        QMessageBox::critical(this, "Error",
            QString("Failed to resolve conflict for '%1'").arg(editorId));
    }
}

void ConflictResolutionDialog::onRemoveBoth()
{
    int row = mConflictList->currentRow();
    if (row < 0 || row >= mConflicts.size())
        return;

    const Data::ConflictInfo& conflict = mConflicts[row];
    QString editorId = conflict.editorId;

    auto reply = QMessageBox::question(this, "Remove Both",
        QString("Are you sure you want to remove both versions of '%1'?\n\nThis action cannot be undone.")
            .arg(editorId),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Remove both versions - need to remove twice since they have different formIds
        removeRecordByPlugin(conflict.type, editorId, conflict.pluginIndexA);
        removeRecordByPlugin(conflict.type, editorId, conflict.pluginIndexB);

        LOG_INFO(QString("Removed both versions of '%1'").arg(editorId));
        mConflicts.removeAt(row);
        loadConflicts();
        QMessageBox::information(this, "Removed",
            QString("Removed both versions of '%1'").arg(editorId));
    }
}

bool ConflictResolutionDialog::removeRecordByPlugin(CkId::Type type, const QString& editorId, int pluginIndex)
{
    // Get formId for the record from the specified plugin
    quint32 formId = getFormIdByPlugin(type, editorId, pluginIndex);
    if (formId == 0) {
        return false;
    }

    // Remove by formId
    return mData->removeRecord(type, QString::number(formId));
}

quint32 ConflictResolutionDialog::getFormIdByPlugin(CkId::Type type, const QString& editorId, int pluginIndex)
{
    switch (type) {
    case CkId::Type_Npc_: {
        const auto& col = mData->getNpcCollection();
        int idx = col.searchId(editorId);
        if (idx >= 0) {
            quint32 formId = col.getRecord(idx).get().formId;
            if (((formId >> 16) & 0xFFFF) == pluginIndex) return formId;
        }
        break;
    }
    case CkId::Type_Weap_: {
        const auto& col = mData->getWeaponCollection();
        int idx = col.searchId(editorId);
        if (idx >= 0) {
            quint32 formId = col.getRecord(idx).get().formId;
            if (((formId >> 16) & 0xFFFF) == pluginIndex) return formId;
        }
        break;
    }
    case CkId::Type_Armor_: {
        const auto& col = mData->getArmorCollection();
        int idx = col.searchId(editorId);
        if (idx >= 0) {
            quint32 formId = col.getRecord(idx).get().formId;
            if (((formId >> 16) & 0xFFFF) == pluginIndex) return formId;
        }
        break;
    }
    case CkId::Type_Spel_: {
        const auto& col = mData->getSpellCollection();
        int idx = col.searchId(editorId);
        if (idx >= 0) {
            quint32 formId = col.getRecord(idx).get().formId;
            if (((formId >> 16) & 0xFFFF) == pluginIndex) return formId;
        }
        break;
    }
    case CkId::Type_Magic_: {
        const auto& col = mData->getMagicCollection();
        int idx = col.searchId(editorId);
        if (idx >= 0) {
            quint32 formId = col.getRecord(idx).get().formId;
            if (((formId >> 16) & 0xFFFF) == pluginIndex) return formId;
        }
        break;
    }
    case CkId::Type_Quest_: {
        const auto& col = mData->getQuestCollection();
        int idx = col.searchId(editorId);
        if (idx >= 0) {
            quint32 formId = col.getRecord(idx).get().formId;
            if (((formId >> 16) & 0xFFFF) == pluginIndex) return formId;
        }
        break;
    }
    case CkId::Type_Dial_: {
        const auto& col = mData->getDialCollection();
        int idx = col.searchId(editorId);
        if (idx >= 0) {
            quint32 formId = col.getRecord(idx).get().formId;
            if (((formId >> 16) & 0xFFFF) == pluginIndex) return formId;
        }
        break;
    }
    case CkId::Type_Info_: {
        const auto& col = mData->getInfoCollection();
        int idx = col.searchId(editorId);
        if (idx >= 0) {
            quint32 formId = col.getRecord(idx).get().formId;
            if (((formId >> 16) & 0xFFFF) == pluginIndex) return formId;
        }
        break;
    }
    default:
        break;
    }

    return 0;
}

void ConflictResolutionDialog::onSaveChanges()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save Conflict Resolution", "",
        "ESM Files (*.esm);;All Files (*)");

    if (filePath.isEmpty())
        return;

    ESMWriter writer;
    QFile saveFile(filePath);
    if (!saveFile.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "Error", QString("Cannot open file: %1").arg(filePath));
        return;
    }

    const auto& metaData = mData->getMetaData().getRecords();
    for (const auto& record : metaData)
    {
        writer.addMaster(record.get().editorId);
    }

    // Count total records across all collections
    int totalRecords = 0;
    totalRecords += mData->getNpcCollection().size();
    totalRecords += mData->getWeaponCollection().size();
    totalRecords += mData->getArmorCollection().size();
    totalRecords += mData->getSpellCollection().size();
    totalRecords += mData->getMagicCollection().size();
    totalRecords += mData->getQuestCollection().size();
    totalRecords += mData->getDialCollection().size();
    totalRecords += mData->getInfoCollection().size();
    totalRecords += mData->getPackCollection().size();
    totalRecords += mData->getTreeCollection().size();
    totalRecords += mData->getAlchCollection().size();
    totalRecords += mData->getIngrCollection().size();
    totalRecords += mData->getContCollection().size();
    totalRecords += mData->getEnchCollection().size();
    totalRecords += mData->getBookCollection().size();
    totalRecords += mData->getMiscCollection().size();
    totalRecords += mData->getActiCollection().size();
    totalRecords += mData->getStatCollection().size();
    totalRecords += mData->getRaceCollection().size();
    totalRecords += mData->getClassCollection().size();
    totalRecords += mData->getFactCollection().size();
    totalRecords += mData->getPerkCollection().size();
    totalRecords += mData->getCellCollection().size();
    totalRecords += mData->getWorldspaceCollection().size();
    totalRecords += mData->getLocationCollection().size();
    totalRecords += mData->getRefrCollection().size();
    totalRecords += mData->getMaterialCollection().size();

    writer.setVersion(1.0f);
    writer.setNumRecords(totalRecords);

    writer.save(saveFile);

    // Write records from each collection
    auto writeCollection = [&writer](const auto& collection, NAME tag) {
        const auto& records = collection.getRecords();
        for (int i = 0; i < records.size(); ++i)
        {
            if (records[i].isErased())
                continue;

            RecHeader header;
            header.id = records[i].get().formId;
            writer.startRecord(tag, header);
            records[i].get().save(writer);
            writer.endRecord();
        }
    };

    writeCollection(mData->getNpcCollection(), 'NPC_');
    writeCollection(mData->getWeaponCollection(), 'WEAP');
    writeCollection(mData->getArmorCollection(), 'ARMO');
    writeCollection(mData->getSpellCollection(), 'SPEL');
    writeCollection(mData->getMagicCollection(), 'MGEF');
    writeCollection(mData->getQuestCollection(), 'QUST');
    writeCollection(mData->getDialCollection(), 'DIAL');
    writeCollection(mData->getInfoCollection(), 'INFO');
    writeCollection(mData->getPackCollection(), 'PACK');
    writeCollection(mData->getTreeCollection(), 'TREE');
    writeCollection(mData->getAlchCollection(), 'ALCH');
    writeCollection(mData->getIngrCollection(), 'INGR');
    writeCollection(mData->getContCollection(), 'CONT');
    writeCollection(mData->getEnchCollection(), 'ENCH');
    writeCollection(mData->getBookCollection(), 'BOOK');
    writeCollection(mData->getMiscCollection(), 'MISC');
    writeCollection(mData->getActiCollection(), 'ACTI');
    writeCollection(mData->getStatCollection(), 'STAT');
    writeCollection(mData->getRaceCollection(), 'RACE');
    writeCollection(mData->getClassCollection(), 'CLAS');
    writeCollection(mData->getFactCollection(), 'FACT');
    writeCollection(mData->getPerkCollection(), 'PERK');
    writeCollection(mData->getCellCollection(), 'CELL');
    writeCollection(mData->getWorldspaceCollection(), 'WRLD');
    writeCollection(mData->getLocationCollection(), 'LCTN');
    writeCollection(mData->getRefrCollection(), 'REFR');
    writeCollection(mData->getMaterialCollection(), 'MATL');
    writeCollection(mData->getLandCollection(), 'LAND');
    writeCollection(mData->getSounCollection(), 'SOUN');
    writeCollection(mData->getWthrCollection(), 'WTHR');
    writeCollection(mData->getLtexCollection(), 'LTEX');

    writer.close();
    saveFile.close();

    int resolvedConflicts = mConflicts.size();
    LOG_INFO(QString("Saved conflict resolution to %1").arg(filePath));
    LOG_INFO(QString("Resolved conflicts: %1, Total records: %2").arg(resolvedConflicts).arg(totalRecords));

    QMessageBox::information(this, "Saved",
        QString("Conflict resolution saved.\n\n"
                "Conflicts resolved: %1\n"
                "Records written: %2")
            .arg(resolvedConflicts).arg(totalRecords));
}
