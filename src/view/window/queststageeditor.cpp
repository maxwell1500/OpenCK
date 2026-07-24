#include "queststageeditor.hpp"
#include "fieldvalidators.hpp"

#include "Questrecord.hpp"
#include "records.hpp"
#include "logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QComboBox>
#include <QMessageBox>
#include <QDataStream>

QuestStageEditor::QuestStageEditor(QuestRecord* quest, QWidget* parent)
    : QDialog(parent),
      mQuest(quest),
      mXpReward(0),
      mGoldReward(0),
      mStagesTable(nullptr),
      mXpSpin(nullptr),
      mGoldSpin(nullptr),
      mItemList(nullptr),
      mSpellList(nullptr),
      mItemFormIdEdit(nullptr),
      mSpellFormIdEdit(nullptr)
{
    setupUI();
    loadFromQuest();
}

void QuestStageEditor::setupUI()
{
    setWindowTitle("Quest Stage Editor");
    setMinimumSize(600, 500);

    auto* mainLayout = new QVBoxLayout(this);

    auto* stagesGroup = new QGroupBox("Stages");
    auto* stagesLayout = new QVBoxLayout(stagesGroup);

    mStagesTable = new QTableWidget(0, 4);
    mStagesTable->setHorizontalHeaderLabels({"Stage Index", "Log Entry", "Objective Text", "Flags"});
    mStagesTable->horizontalHeader()->setStretchLastSection(true);
    mStagesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    mStagesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    mStagesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    mStagesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    stagesLayout->addWidget(mStagesTable);

    auto* stageBtnLayout = new QHBoxLayout();
    auto* addStageBtn = new QPushButton("Add Stage");
    auto* removeStageBtn = new QPushButton("Remove Stage");
    stageBtnLayout->addWidget(addStageBtn);
    stageBtnLayout->addWidget(removeStageBtn);
    stageBtnLayout->addStretch();
    stagesLayout->addLayout(stageBtnLayout);

    connect(addStageBtn, &QPushButton::clicked, this, &QuestStageEditor::addStage);
    connect(removeStageBtn, &QPushButton::clicked, this, &QuestStageEditor::removeStage);

    mainLayout->addWidget(stagesGroup);

    auto* rewardsGroup = new QGroupBox("Rewards");
    auto* rewardsLayout = new QVBoxLayout(rewardsGroup);

    auto* rewardFormLayout = new QFormLayout();
    mXpSpin = new QSpinBox();
    mXpSpin->setRange(0, 999999);
    rewardFormLayout->addRow("XP Reward:", mXpSpin);

    mGoldSpin = new QSpinBox();
    mGoldSpin->setRange(0, 999999);
    rewardFormLayout->addRow("Gold Reward:", mGoldSpin);
    rewardsLayout->addLayout(rewardFormLayout);

    auto* itemLayout = new QHBoxLayout();
    auto* itemLeftLayout = new QVBoxLayout();
    auto* itemLabel = new QLabel("Item Rewards:");
    mItemList = new QListWidget();
    mItemList->setMaximumHeight(100);
    itemLeftLayout->addWidget(itemLabel);
    itemLeftLayout->addWidget(mItemList);
    auto* itemRightLayout = new QVBoxLayout();
    mItemFormIdEdit = new QLineEdit();
    mItemFormIdEdit->setPlaceholderText("FormID (hex)");
    setHexFormIdValidator(mItemFormIdEdit, this);
    auto* addItemBtn = new QPushButton("Add Item");
    auto* removeItemBtn = new QPushButton("Remove Item");
    itemRightLayout->addWidget(mItemFormIdEdit);
    itemRightLayout->addWidget(addItemBtn);
    itemRightLayout->addWidget(removeItemBtn);
    itemRightLayout->addStretch();
    itemLayout->addLayout(itemLeftLayout, 1);
    itemLayout->addLayout(itemRightLayout);
    rewardsLayout->addLayout(itemLayout);

    connect(addItemBtn, &QPushButton::clicked, this, &QuestStageEditor::addItemReward);
    connect(removeItemBtn, &QPushButton::clicked, this, &QuestStageEditor::removeItemReward);

    auto* spellLayout = new QHBoxLayout();
    auto* spellLeftLayout = new QVBoxLayout();
    auto* spellLabel = new QLabel("Spell Rewards:");
    mSpellList = new QListWidget();
    mSpellList->setMaximumHeight(100);
    spellLeftLayout->addWidget(spellLabel);
    spellLeftLayout->addWidget(mSpellList);
    auto* spellRightLayout = new QVBoxLayout();
    mSpellFormIdEdit = new QLineEdit();
    mSpellFormIdEdit->setPlaceholderText("FormID (hex)");
    setHexFormIdValidator(mSpellFormIdEdit, this);
    auto* addSpellBtn = new QPushButton("Add Spell");
    auto* removeSpellBtn = new QPushButton("Remove Spell");
    spellRightLayout->addWidget(mSpellFormIdEdit);
    spellRightLayout->addWidget(addSpellBtn);
    spellRightLayout->addWidget(removeSpellBtn);
    spellRightLayout->addStretch();
    spellLayout->addLayout(spellLeftLayout, 1);
    spellLayout->addLayout(spellRightLayout);
    rewardsLayout->addLayout(spellLayout);

    connect(addSpellBtn, &QPushButton::clicked, this, &QuestStageEditor::addSpellReward);
    connect(removeSpellBtn, &QPushButton::clicked, this, &QuestStageEditor::removeSpellReward);

    mainLayout->addWidget(rewardsGroup);

    auto* buttonLayout = new QHBoxLayout();
    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &QuestStageEditor::saveChanges);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void QuestStageEditor::loadFromQuest()
{
    mStageIndices.clear();
    mStageTexts.clear();
    mObjectiveTexts.clear();
    mStageFlags.clear();
    mXpReward = 0;
    mGoldReward = 0;
    mItemRewards.clear();
    mSpellRewards.clear();

    for (int i = 0; i < mQuest->stageIds.size(); ++i)
    {
        mStageIndices.append(mQuest->stageIds[i]);
        if (i < mQuest->stageDescriptions.size())
            mStageTexts.append(mQuest->stageDescriptions[i]);
        else
            mStageTexts.append(QString());
        mStageFlags.append(0);
    }

    for (const auto& raw : mQuest->rawSubRecords)
    {
        if (raw.name == 'SFLG')
        {
            QDataStream stream(raw.data);
            stream.setByteOrder(QDataStream::LittleEndian);
            quint32 count = 0;
            stream >> count;
            mStageFlags.resize(count);
            for (quint32 j = 0; j < count; ++j)
                stream >> mStageFlags[j];
        }
        else if (raw.name == 'REXP')
        {
            QDataStream stream(raw.data);
            stream.setByteOrder(QDataStream::LittleEndian);
            stream >> mXpReward;
        }
        else if (raw.name == 'RGOL')
        {
            QDataStream stream(raw.data);
            stream.setByteOrder(QDataStream::LittleEndian);
            stream >> mGoldReward;
        }
        else if (raw.name == 'RITM')
        {
            QDataStream stream(raw.data);
            stream.setByteOrder(QDataStream::LittleEndian);
            quint32 count = 0;
            stream >> count;
            mItemRewards.resize(count);
            for (quint32 j = 0; j < count; ++j)
                stream >> mItemRewards[j];
        }
        else if (raw.name == 'RSPL')
        {
            QDataStream stream(raw.data);
            stream.setByteOrder(QDataStream::LittleEndian);
            quint32 count = 0;
            stream >> count;
            mSpellRewards.resize(count);
            for (quint32 j = 0; j < count; ++j)
                stream >> mSpellRewards[j];
        }
    }

    if (mStageFlags.size() < mStageIndices.size())
        mStageFlags.resize(mStageIndices.size());

    mStagesTable->setRowCount(0);
    for (int i = 0; i < mStageIndices.size(); ++i)
    {
        int row = mStagesTable->rowCount();
        mStagesTable->insertRow(row);

        auto* indexItem = new QTableWidgetItem(QString::number(mStageIndices[i]));
        mStagesTable->setItem(row, 0, indexItem);

        auto* textItem = new QTableWidgetItem(mStageTexts[i]);
        mStagesTable->setItem(row, 1, textItem);

        auto* objectiveItem = new QTableWidgetItem(i < mObjectiveTexts.size() ? mObjectiveTexts[i] : QString());
        mStagesTable->setItem(row, 2, objectiveItem);

        auto* flagCombo = new QComboBox();
        flagCombo->addItem("None", 0U);
        flagCombo->addItem("Start Stage", 1U);
        flagCombo->addItem("Finish Stage", 2U);
        flagCombo->addItem("Complete Quest", 3U);
        flagCombo->addItem("Fail Stage", 4U);
        int flagIdx = flagCombo->findData(mStageFlags[i]);
        if (flagIdx >= 0)
            flagCombo->setCurrentIndex(flagIdx);
        mStagesTable->setCellWidget(row, 3, flagCombo);
    }

    mXpSpin->setValue(mXpReward);
    mGoldSpin->setValue(mGoldReward);

    mItemList->clear();
    for (quint32 formId : mItemRewards)
        mItemList->addItem(QString("0x%1").arg(formId, 8, 16, QChar('0')).toUpper());

    mSpellList->clear();
    for (quint32 formId : mSpellRewards)
        mSpellList->addItem(QString("0x%1").arg(formId, 8, 16, QChar('0')).toUpper());
}

void QuestStageEditor::saveToQuest()
{
    mStageIndices.clear();
    mStageTexts.clear();
    mObjectiveTexts.clear();
    mStageFlags.clear();

    for (int row = 0; row < mStagesTable->rowCount(); ++row)
    {
        auto* indexItem = mStagesTable->item(row, 0);
        auto* textItem = mStagesTable->item(row, 1);
        auto* objectiveItem = mStagesTable->item(row, 2);
        auto* flagCombo = qobject_cast<QComboBox*>(mStagesTable->cellWidget(row, 3));

        {
            bool ok = false;
            quint32 idx = indexItem ? indexItem->text().toUInt(&ok) : 0;
            mStageIndices.append(ok ? idx : 0);
        }
        mStageTexts.append(textItem ? textItem->text() : QString());
        mObjectiveTexts.append(objectiveItem ? objectiveItem->text() : QString());
        mStageFlags.append(flagCombo ? flagCombo->currentData().toUInt() : 0);
    }

    mQuest->stageIds = mStageIndices;
    mQuest->stageDescriptions = mStageTexts;

    mXpReward = mXpSpin->value();
    mGoldReward = mGoldSpin->value();

    mItemRewards.clear();
    for (int i = 0; i < mItemList->count(); ++i)
    {
        QString text = mItemList->item(i)->text();
        bool ok = false;
        quint32 formId = text.toUInt(&ok, 16);
        if (ok)
            mItemRewards.append(formId);
    }

    mSpellRewards.clear();
    for (int i = 0; i < mSpellList->count(); ++i)
    {
        QString text = mSpellList->item(i)->text();
        bool ok = false;
        quint32 formId = text.toUInt(&ok, 16);
        if (ok)
            mSpellRewards.append(formId);
    }

    auto removeRaw = [&](NAME name) {
        for (int i = mQuest->rawSubRecords.size() - 1; i >= 0; --i)
        {
            if (mQuest->rawSubRecords[i].name == name)
                mQuest->rawSubRecords.removeAt(i);
        }
    };

    removeRaw('SFLG');
    removeRaw('REXP');
    removeRaw('RGOL');
    removeRaw('RITM');
    removeRaw('RSPL');

    {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << (quint32)mStageFlags.size();
        for (quint32 f : mStageFlags)
            stream << f;
        mQuest->rawSubRecords.append({'SFLG', data});
    }

    {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << mXpReward;
        mQuest->rawSubRecords.append({'REXP', data});
    }

    {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << mGoldReward;
        mQuest->rawSubRecords.append({'RGOL', data});
    }

    {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << (quint32)mItemRewards.size();
        for (quint32 id : mItemRewards)
            stream << id;
        mQuest->rawSubRecords.append({'RITM', data});
    }

    {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << (quint32)mSpellRewards.size();
        for (quint32 id : mSpellRewards)
            stream << id;
        mQuest->rawSubRecords.append({'RSPL', data});
    }
}

void QuestStageEditor::addStage()
{
    int row = mStagesTable->rowCount();
    mStagesTable->insertRow(row);

    auto* indexItem = new QTableWidgetItem(QString::number(row));
    mStagesTable->setItem(row, 0, indexItem);

    mStagesTable->setItem(row, 1, new QTableWidgetItem(QString()));
    mStagesTable->setItem(row, 2, new QTableWidgetItem(QString()));

    auto* flagCombo = new QComboBox();
    flagCombo->addItem("None", 0U);
    flagCombo->addItem("Start Stage", 1U);
    flagCombo->addItem("Finish Stage", 2U);
    flagCombo->addItem("Complete Quest", 3U);
    flagCombo->addItem("Fail Stage", 4U);
    mStagesTable->setCellWidget(row, 3, flagCombo);
}

void QuestStageEditor::removeStage()
{
    int row = mStagesTable->currentRow();
    if (row >= 0)
        mStagesTable->removeRow(row);

    for (int i = 0; i < mStagesTable->rowCount(); ++i)
    {
        auto* item = mStagesTable->item(i, 0);
        if (item)
            item->setText(QString::number(i));
    }
}

void QuestStageEditor::addItemReward()
{
    QString text = mItemFormIdEdit->text().trimmed();
    if (text.isEmpty())
        return;

    bool ok = false;
    text.toUInt(&ok, 16);
    if (!ok)
    {
        QMessageBox::warning(this, "Invalid FormID", "Enter a valid hexadecimal FormID.");
        return;
    }

    mItemList->addItem(QString("0x%1").arg(text.toUInt(nullptr, 16), 8, 16, QChar('0')).toUpper());
    mItemFormIdEdit->clear();
}

void QuestStageEditor::removeItemReward()
{
    int row = mItemList->currentRow();
    if (row >= 0)
        delete mItemList->takeItem(row);
}

void QuestStageEditor::addSpellReward()
{
    QString text = mSpellFormIdEdit->text().trimmed();
    if (text.isEmpty())
        return;

    bool ok = false;
    text.toUInt(&ok, 16);
    if (!ok)
    {
        QMessageBox::warning(this, "Invalid FormID", "Enter a valid hexadecimal FormID.");
        return;
    }

    mSpellList->addItem(QString("0x%1").arg(text.toUInt(nullptr, 16), 8, 16, QChar('0')).toUpper());
    mSpellFormIdEdit->clear();
}

void QuestStageEditor::removeSpellReward()
{
    int row = mSpellList->currentRow();
    if (row >= 0)
        delete mSpellList->takeItem(row);
}

void QuestStageEditor::saveChanges()
{
    saveToQuest();
    accept();
}
