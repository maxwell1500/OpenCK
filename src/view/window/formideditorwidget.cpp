#include "formideditorwidget.hpp"
#include "fieldvalidators.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/world/collection.hpp"
#include "../../model/world/record.hpp"
#include "../../model/world/ckid.hpp"
#include "logger.hpp"
#include <QHeaderView>
#include <QMap>

#include "../../../libs/files/esm/npcrecord.hpp"
#include "../../../libs/files/esm/weaprecord.hpp"
#include "../../../libs/files/esm/armorrecord.hpp"
#include "../../../libs/files/esm/spellrecord.hpp"
#include "../../../libs/files/esm/questrecord.hpp"

FormIdEditorWidget::FormIdEditorWidget(Data* data, QWidget* parent)
    : QWidget(parent)
    , mData(data)
    , searchEdit(nullptr)
    , conflictLabel(nullptr)
    , recordInfoLabel(nullptr)
    , conflictTable(nullptr)
    , searchButton(nullptr)
    , conflictButton(nullptr)
{
    LOG_DEBUG("FormIdEditorWidget created");
    setupUI();
}

FormIdEditorWidget::~FormIdEditorWidget()
{
    LOG_DEBUG("FormIdEditorWidget destroyed");
}

void FormIdEditorWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel("FormID:", this));
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Enter FormID (e.g., 0x00012345)");
    setFormIdValidator(searchEdit, this);
    searchLayout->addWidget(searchEdit);
    searchButton = new QPushButton("Search", this);
    searchLayout->addWidget(searchButton);
    mainLayout->addLayout(searchLayout);

    recordInfoLabel = new QLabel("Record info will appear here", this);
    mainLayout->addWidget(recordInfoLabel);

    conflictButton = new QPushButton("Check Conflicts", this);
    mainLayout->addWidget(conflictButton);

    conflictLabel = new QLabel("No conflicts detected", this);
    mainLayout->addWidget(conflictLabel);

    conflictTable = new QTableWidget(this);
    conflictTable->setColumnCount(3);
    conflictTable->setHorizontalHeaderLabels(QStringList() << "Plugin" << "Record Type" << "Editor ID");
    conflictTable->horizontalHeader()->setStretchLastSection(true);
    mainLayout->addWidget(conflictTable);

    connect(searchButton, &QPushButton::clicked, this, &FormIdEditorWidget::onSearch);
    connect(conflictButton, &QPushButton::clicked, this, &FormIdEditorWidget::onConflictCheck);
    connect(searchEdit, &QLineEdit::textChanged, this, &FormIdEditorWidget::onFormIdEdited);
}

void FormIdEditorWidget::onSearch()
{
    QString formId = searchEdit->text().trimmed();
    if (formId.isEmpty()) {
        LOG_WARNING("Empty FormID search");
        return;
    }

    LOG_INFO(QString("Searching for FormID: %1").arg(formId));

    if (!mData) {
        recordInfoLabel->setText("No data loaded");
        return;
    }

    bool ok;
    quint32 id = formId.toUInt(&ok, 16);
    if (!ok) {
        recordInfoLabel->setText("Invalid FormID format (use hex like 0x00012345)");
        return;
    }

    recordInfoLabel->setText(QString("FormID: 0x%1").arg(id, 8, 16, QChar('0')));
    updateConflictInfo(formId);
}

void FormIdEditorWidget::onConflictCheck()
{
    QString formId = searchEdit->text().trimmed();
    if (formId.isEmpty()) {
        LOG_WARNING("No FormID to check for conflicts");
        return;
    }
    updateConflictInfo(formId);
}

void FormIdEditorWidget::onFormIdEdited(const QString& text)
{
    QString trimmed = text.trimmed();
    if (!trimmed.isEmpty())
    {
        updateConflictInfo(trimmed);
    }
    else
    {
        conflictLabel->setText("Enter a FormID to check for conflicts");
        conflictTable->setRowCount(0);
    }
}

void FormIdEditorWidget::updateConflictInfo(const QString& formId)
{
    LOG_INFO(QString("Checking conflicts for FormID: %1").arg(formId));

    bool ok;
    quint32 formIdValue = formId.isEmpty() ? 0 : formId.toUInt(&ok, 16);
    
    if (!ok)
    {
        conflictLabel->setText(QString("Invalid FormID: %1").arg(formId));
        conflictTable->setRowCount(0);
        return;
    }

    int pluginIndex = (formIdValue >> 16) & 0xFFFF;
    int recordIndex = formIdValue & 0xFFFF;

    conflictTable->setRowCount(0);

    QList<QStringList> foundRecords;
    QStringList files = mData->getContentFiles();

    // Check each record type by iterating and finding matches
    auto& npcCol = mData->getNpcCollection();
    for (int i = 0; i < npcCol.size(); i++)
    {
        const NpcRecord& npc = npcCol.getRecord(i).get();
        if ((npc.formId & 0x0000FFFF) == (formIdValue & 0x0000FFFF))
        {
            int pIdx = (npc.formId >> 16) & 0xFFFF;
            QString fileName = (pIdx > 0 && pIdx - 1 < files.size()) ? files[pIdx - 1] : QString("Base (0x%1)").arg(pIdx, 4, 16, QChar('0'));
            foundRecords << QStringList({QString("NPC_"), npc.editorId, QString("0x%1").arg(npc.formId, 8, 16, QChar('0')), fileName});
        }
    }

    auto& weapCol = mData->getWeaponCollection();
    for (int i = 0; i < weapCol.size(); i++)
    {
        const WeaponRecord& weap = weapCol.getRecord(i).get();
        if ((weap.formId & 0x0000FFFF) == (formIdValue & 0x0000FFFF))
        {
            int pIdx = (weap.formId >> 16) & 0xFFFF;
            QString fileName = (pIdx > 0 && pIdx - 1 < files.size()) ? files[pIdx - 1] : QString("Base (0x%1)").arg(pIdx, 4, 16, QChar('0'));
            foundRecords << QStringList({QString("WEAP_"), weap.editorId, QString("0x%1").arg(weap.formId, 8, 16, QChar('0')), fileName});
        }
    }

    auto& armrCol = mData->getArmorCollection();
    for (int i = 0; i < armrCol.size(); i++)
    {
        const ArmorRecord& armr = armrCol.getRecord(i).get();
        if ((armr.formId & 0x0000FFFF) == (formIdValue & 0x0000FFFF))
        {
            int pIdx = (armr.formId >> 16) & 0xFFFF;
            QString fileName = (pIdx > 0 && pIdx - 1 < files.size()) ? files[pIdx - 1] : QString("Base (0x%1)").arg(pIdx, 4, 16, QChar('0'));
            foundRecords << QStringList({QString("ARMOR_"), armr.editorId, QString("0x%1").arg(armr.formId, 8, 16, QChar('0')), fileName});
        }
    }

    auto& spelCol = mData->getSpellCollection();
    for (int i = 0; i < spelCol.size(); i++)
    {
        const SpellRecord& spel = spelCol.getRecord(i).get();
        if ((spel.formId & 0x0000FFFF) == (formIdValue & 0x0000FFFF))
        {
            int pIdx = (spel.formId >> 16) & 0xFFFF;
            QString fileName = (pIdx > 0 && pIdx - 1 < files.size()) ? files[pIdx - 1] : QString("Base (0x%1)").arg(pIdx, 4, 16, QChar('0'));
            foundRecords << QStringList({QString("SPEL_"), spel.editorId, QString("0x%1").arg(spel.formId, 8, 16, QChar('0')), fileName});
        }
    }

    auto& questCol = mData->getQuestCollection();
    for (int i = 0; i < questCol.size(); i++)
    {
        const QuestRecord& quest = questCol.getRecord(i).get();
        if ((quest.formId & 0x0000FFFF) == (formIdValue & 0x0000FFFF))
        {
            int pIdx = (quest.formId >> 16) & 0xFFFF;
            QString fileName = (pIdx > 0 && pIdx - 1 < files.size()) ? files[pIdx - 1] : QString("Base (0x%1)").arg(pIdx, 4, 16, QChar('0'));
            foundRecords << QStringList({QString("QUST_"), quest.editorId, QString("0x%1").arg(quest.formId, 8, 16, QChar('0')), fileName});
        }
    }

    // Display results
    conflictTable->setHorizontalHeaderLabels(QStringList() << "Type" << "EditorID" << "FormID" << "Plugin");

    if (foundRecords.isEmpty())
    {
        conflictLabel->setText(QString("FormID %1: No records found matching these bottom 2 bytes").arg(formId));
    }
    else
    {
        // Group by bottom 2 bytes to find actual conflicts
        QMap<int, QList<int>> conflictsByIndex;
        for (int i = 0; i < foundRecords.size(); i++)
        {
            int idx = foundRecords[i][2].toInt(nullptr, 16) & 0xFFFF;
            conflictsByIndex[idx].append(i);
        }

        int conflictCount = 0;
        for (auto it = conflictsByIndex.begin(); it != conflictsByIndex.end(); ++it)
        {
            if (it.value().size() > 1)
            {
                conflictCount++;
                for (int idx : it.value())
                {
                    conflictTable->insertRow(conflictTable->rowCount());
                    for (int c = 0; c < 4; c++)
                    {
                        conflictTable->setItem(conflictTable->rowCount() - 1, c,
                            new QTableWidgetItem(foundRecords[idx][c]));
                    }
                }
            }
        }

        if (conflictCount > 0)
        {
            conflictLabel->setText(QString("FormID %1: %2 potential conflict(s) found - records with same bottom bytes in different plugins").arg(formId).arg(conflictCount));
        }
        else
        {
            // Show all matching records
            for (const QStringList& rec : foundRecords)
            {
                conflictTable->insertRow(conflictTable->rowCount());
                for (int c = 0; c < 4; c++)
                {
                    conflictTable->setItem(conflictTable->rowCount() - 1, c,
                        new QTableWidgetItem(rec[c]));
                }
            }
            conflictLabel->setText(QString("FormID %1: %2 record(s) found (no conflicts - different bottom bytes)").arg(formId).arg(foundRecords.size()));
        }
    }
}
