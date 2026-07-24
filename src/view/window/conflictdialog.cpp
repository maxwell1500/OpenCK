#include "conflictdialog.hpp"
#include "../../model/world/data.hpp"
#include "logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTextStream>
#include <QScrollBar>
#include <QFileDialog>
#include <QFile>
#include <QMap>
#include <QSet>

#include "../../../libs/files/esm/npcrecord.hpp"
#include "../../../libs/files/esm/spellrecord.hpp"

ConflictDialog::ConflictDialog(Data* data, QWidget* parent) :
    QDialog(parent),
    mData(data),
    conflictsList(new QListWidget(this)),
    statusLabel(new QLabel("Conflict Detection", this))
{
    setWindowTitle("Conflict Detection");
    setMinimumSize(700, 500);
    
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(statusLabel);
    layout->addWidget(conflictsList);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Save, this);
    auto* saveBtn = buttonBox->button(QDialogButtonBox::Save);
    saveBtn->setText("Save Report");
    layout->addWidget(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        saveReport();
    });
    
    checkConflicts();
}

ConflictDialog::~ConflictDialog()
{
}

void ConflictDialog::saveReport()
{
    QString report;
    QTextStream out(&report);
    
    for (int i = 0; i < conflictsList->count(); i++)
    {
        out << conflictsList->item(i)->text() << "\n";
    }
    
    QString filePath = QFileDialog::getSaveFileName(this, "Save Conflict Report", "", "Text Files (*.txt)");
    if (!filePath.isEmpty())
    {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            file.write(report.toUtf8());
            LOG_INFO(QString("Conflict report saved to %1").arg(filePath));
        }
    }
}

void ConflictDialog::checkConflicts()
{
    LOG_DEBUG("Checking for conflicts");
    conflictsList->clear();
    
    if (!mData)
    {
        statusLabel->setText("No data available");
        conflictsList->addItem("No data available");
        LOG_WARNING("No data available for conflict detection");
        return;
    }
    
    QStringList files = mData->getContentFiles();
    int fileCount = files.size();
    
    conflictsList->addItem("=" + QString(60, QChar('=')) + "=");
    conflictsList->addItem(QString("Conflict Detection Report - %1 plugin(s) loaded").arg(fileCount));
    conflictsList->addItem("=" + QString(60, QChar('=')) + "=");
    
    for (int i = 0; i < fileCount; i++)
    {
        conflictsList->addItem(QString("  [%1] %2").arg(i).arg(files.at(i)));
    }
    conflictsList->addItem("");
    
    int conflictCount = 0;
    int warningCount = 0;
    
    // Check FormID conflicts: same bottom 2 bytes across different plugins
    QList<QString> npcIds = mData->getNpcCollection().getIds(false);
    QList<QString> weapIds = mData->getWeaponCollection().getIds(false);
    QList<QString> armrIds = mData->getArmorCollection().getIds(false);
    QList<QString> spelIds = mData->getSpellCollection().getIds(false);
    QList<QString> questIds = mData->getQuestCollection().getIds(false);
    QList<QString> alchIds = mData->getAlchCollection().getIds(false);
    QList<QString> ingrIds = mData->getIngrCollection().getIds(false);
    QList<QString> bookIds = mData->getBookCollection().getIds(false);
    QList<QString> enchIds = mData->getEnchCollection().getIds(false);
    QList<QString> contIds = mData->getContCollection().getIds(false);
    QList<QString> miscIds = mData->getMiscCollection().getIds(false);
    QList<QString> actiIds = mData->getActiCollection().getIds(false);
    QList<QString> statIds = mData->getStatCollection().getIds(false);
    QList<QString> packIds = mData->getPackCollection().getIds(false);
    QList<QString> raceIds = mData->getRaceCollection().getIds(false);
    QList<QString> classIds = mData->getClassCollection().getIds(false);
    QList<QString> factIds = mData->getFactCollection().getIds(false);
    QList<QString> perkIds = mData->getPerkCollection().getIds(false);
    
    // Check for editor ID collisions across collections of the same type
    QList<QPair<QString, QList<QString>>> allCollections;
    allCollections << qMakePair(QString("NPC_"), npcIds);
    allCollections << qMakePair(QString("WEAP_"), weapIds);
    allCollections << qMakePair(QString("ARMOR_"), armrIds);
    allCollections << qMakePair(QString("SPEL_"), spelIds);
    allCollections << qMakePair(QString("QUST_"), questIds);
    allCollections << qMakePair(QString("ALCH_"), alchIds);
    allCollections << qMakePair(QString("INGR_"), ingrIds);
    allCollections << qMakePair(QString("BOOK_"), bookIds);
    allCollections << qMakePair(QString("ENCH_"), enchIds);
    allCollections << qMakePair(QString("CONT_"), contIds);
    allCollections << qMakePair(QString("MISC_"), miscIds);
    allCollections << qMakePair(QString("ACTI_"), actiIds);
    allCollections << qMakePair(QString("STAT_"), statIds);
    allCollections << qMakePair(QString("PACK_"), packIds);
    allCollections << qMakePair(QString("RACE_"), raceIds);
    allCollections << qMakePair(QString("CLASS_"), classIds);
    allCollections << qMakePair(QString("FACT_"), factIds);
    allCollections << qMakePair(QString("PERK_"), perkIds);
    
    // Check each collection for records from multiple plugins
    for (const auto& pair : allCollections)
    {
        const QString& typeName = pair.first;
        const QList<QString>& ids = pair.second;
        
        if (ids.size() <= 1)
            continue;
        
        QMap<int, QStringList> pluginMap;
        
        for (const QString& id : ids)
        {
            // Get the FormID for this record and extract plugin index
            quint32 formId = getFormIdForRecord(typeName, id);
            if (formId != 0)
            {
                int pluginIndex = (formId >> 16) & 0xFFFF;
                pluginMap[pluginIndex].append(id);
            }
        }
        
        // If we have records from multiple plugins, flag the ones that exist in non-base plugins
        if (pluginMap.size() > 1)
        {
            for (auto it = pluginMap.begin(); it != pluginMap.end(); ++it)
            {
                if (it.key() > 0 && it.value().size() > 0)
                {
                    QString pluginName = (it.key() - 1 < fileCount) ? files[it.key() - 1] : QString("Plugin %1").arg(it.key());
                    for (const QString& recId : it.value())
                    {
                        QString msg = QString("[Conflict] %1 '%2' exists in %3")
                            .arg(typeName, recId, pluginName);
                        conflictsList->addItem(msg);
                        conflictCount++;
                    }
                }
            }
        }
    }
    
    // Check for NPC records that reference non-existent spells
    QList<QString> allSpellIds = mData->getSpellCollection().getIds(false);
    QSet<QString> spellIdSet(allSpellIds.begin(), allSpellIds.end());
    
    if (!spellIdSet.isEmpty() && !npcIds.isEmpty())
    {
        auto& npcCollection = mData->getNpcCollection();
        for (int i = 0; i < npcCollection.size(); i++)
        {
            const NpcRecord& npc = npcCollection.getRecord(i).get();
            for (quint32 spellFormId : npc.spells)
            {
                QString spellEditorId;
                // Find the spell by formID
                for (const QString& sid : allSpellIds)
                {
                    int idx = mData->getSpellCollection().getIndex(sid);
                    if (idx >= 0)
                    {
                        const SpellRecord& sr = mData->getSpellCollection().getRecord(idx).get();
                        if (sr.formId == spellFormId)
                        {
                            spellEditorId = sid;
                            break;
                        }
                    }
                }
                if (spellEditorId.isEmpty())
                {
                    QString msg = QString("[Orphan] NPC '%1' references unknown spell 0x%2")
                        .arg(npc.editorId, QString::number(spellFormId, 16).toUpper().rightJustified(8, '0'));
                    conflictsList->addItem(msg);
                    warningCount++;
                }
            }
        }
    }
    
    conflictsList->addItem("");
    conflictsList->addItem("=" + QString(60, QChar('=')) + "=");
    conflictsList->addItem(QString("Summary: %1 conflict(s), %2 warning(s)").arg(conflictCount).arg(warningCount));
    conflictsList->addItem("=" + QString(60, QChar('=')) + "=");
    
    if (conflictCount == 0 && warningCount == 0)
    {
        conflictsList->addItem("No cross-plugin conflicts or orphans detected.");
    }
    else if (conflictCount > 0)
    {
        conflictsList->addItem("WARNING: Records with the same editor ID may conflict at runtime.");
        conflictsList->addItem("Review the load order and ensure only one plugin defines each record.");
    }
    
    statusLabel->setText(QString("Found %1 conflict(s), %2 warning(s)").arg(conflictCount).arg(warningCount));
    LOG_INFO(QString("Conflict detection complete: %1 conflicts, %2 warnings").arg(conflictCount).arg(warningCount));
}

quint32 ConflictDialog::getFormIdForRecord(const QString& typeName, const QString& editorId)
{
    if (typeName == "NPC_")
    {
        int idx = mData->getNpcCollection().getIndex(editorId);
        if (idx >= 0) return mData->getNpcCollection().getRecord(idx).get().formId;
    }
    else if (typeName == "WEAP_")
    {
        int idx = mData->getWeaponCollection().getIndex(editorId);
        if (idx >= 0) return mData->getWeaponCollection().getRecord(idx).get().formId;
    }
    else if (typeName == "ARMOR_")
    {
        int idx = mData->getArmorCollection().getIndex(editorId);
        if (idx >= 0) return mData->getArmorCollection().getRecord(idx).get().formId;
    }
    else if (typeName == "SPEL_")
    {
        int idx = mData->getSpellCollection().getIndex(editorId);
        if (idx >= 0) return mData->getSpellCollection().getRecord(idx).get().formId;
    }
    else if (typeName == "QUST_")
    {
        int idx = mData->getQuestCollection().getIndex(editorId);
        if (idx >= 0) return mData->getQuestCollection().getRecord(idx).get().formId;
    }
    else if (typeName == "ALCH_")
    {
        int idx = mData->getAlchCollection().getIndex(editorId);
        if (idx >= 0) return mData->getAlchCollection().getRecord(idx).get().formId;
    }
    else if (typeName == "INGR_")
    {
        int idx = mData->getIngrCollection().getIndex(editorId);
        if (idx >= 0) return mData->getIngrCollection().getRecord(idx).get().formId;
    }
    else if (typeName == "BOOK_")
    {
        int idx = mData->getBookCollection().getIndex(editorId);
        if (idx >= 0) return mData->getBookCollection().getRecord(idx).get().formId;
    }
    else if (typeName == "ENCH_")
    {
        int idx = mData->getEnchCollection().getIndex(editorId);
        if (idx >= 0) return mData->getEnchCollection().getRecord(idx).get().formId;
    }
    else if (typeName == "CONT_")
    {
        int idx = mData->getContCollection().getIndex(editorId);
        if (idx >= 0) return mData->getContCollection().getRecord(idx).get().formId;
    }
    else if (typeName == "MISC_")
    {
        int idx = mData->getMiscCollection().getIndex(editorId);
        if (idx >= 0) return mData->getMiscCollection().getRecord(idx).get().formId;
    }
    else if (typeName == "ACTI_")
    {
        int idx = mData->getActiCollection().getIndex(editorId);
        if (idx >= 0) return mData->getActiCollection().getRecord(idx).get().formId;
    }
    else if (typeName == "STAT_")
    {
        int idx = mData->getStatCollection().getIndex(editorId);
        if (idx >= 0) return mData->getStatCollection().getRecord(idx).get().formId;
    }
    else if (typeName == "PACK_")
    {
        int idx = mData->getPackCollection().getIndex(editorId);
        if (idx >= 0) return mData->getPackCollection().getRecord(idx).get().formId;
    }
    else if (typeName == "RACE_")
    {
        int idx = mData->getRaceCollection().getIndex(editorId);
        if (idx >= 0) return mData->getRaceCollection().getRecord(idx).get().formId;
    }
    else if (typeName == "CLASS_")
    {
        int idx = mData->getClassCollection().getIndex(editorId);
        if (idx >= 0) return mData->getClassCollection().getRecord(idx).get().formId;
    }
    else if (typeName == "FACT_")
    {
        int idx = mData->getFactCollection().getIndex(editorId);
        if (idx >= 0) return mData->getFactCollection().getRecord(idx).get().formId;
    }
    else if (typeName == "PERK_")
    {
        int idx = mData->getPerkCollection().getIndex(editorId);
        if (idx >= 0) return mData->getPerkCollection().getRecord(idx).get().formId;
    }
    return 0;
}
