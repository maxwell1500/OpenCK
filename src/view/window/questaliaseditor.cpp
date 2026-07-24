#include "questaliaseditor.hpp"
#include "fieldvalidators.hpp"

#include "Questrecord.hpp"
#include "records.hpp"
#include "logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QHeaderView>
#include <QDataStream>
#include <QMessageBox>

QuestAliasEditor::QuestAliasEditor(QuestRecord* quest, QWidget* parent)
    : QDialog(parent),
      mQuest(quest),
      mAliasTable(nullptr)
{
    setupUI();
    loadFromQuest();
}

void QuestAliasEditor::setupUI()
{
    setWindowTitle("Quest Alias Editor");
    setMinimumSize(700, 400);

    auto* mainLayout = new QVBoxLayout(this);

    auto* aliasGroup = new QGroupBox("Aliases");
    auto* aliasLayout = new QVBoxLayout(aliasGroup);

    mAliasTable = new QTableWidget(0, 4);
    mAliasTable->setHorizontalHeaderLabels({"Alias Name", "Type", "Reference FormID", "Is Optional"});
    mAliasTable->horizontalHeader()->setStretchLastSection(true);
    mAliasTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    mAliasTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    mAliasTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    mAliasTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    mAliasTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    aliasLayout->addWidget(mAliasTable);

    auto* aliasBtnLayout = new QHBoxLayout();
    auto* addAliasBtn = new QPushButton("Add Alias");
    auto* removeAliasBtn = new QPushButton("Remove Alias");
    aliasBtnLayout->addWidget(addAliasBtn);
    aliasBtnLayout->addWidget(removeAliasBtn);
    aliasBtnLayout->addStretch();
    aliasLayout->addLayout(aliasBtnLayout);

    connect(addAliasBtn, &QPushButton::clicked, this, &QuestAliasEditor::addAlias);
    connect(removeAliasBtn, &QPushButton::clicked, this, &QuestAliasEditor::removeAlias);

    mainLayout->addWidget(aliasGroup);

    auto* buttonLayout = new QHBoxLayout();
    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &QuestAliasEditor::saveChanges);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void QuestAliasEditor::loadFromQuest()
{
    mAliases.clear();

    for (const auto& raw : mQuest->rawSubRecords)
    {
        if (raw.name == 'ALIA')
        {
            QDataStream stream(raw.data);
            stream.setByteOrder(QDataStream::LittleEndian);
            quint32 count = 0;
            stream >> count;
            for (quint32 i = 0; i < count; ++i)
            {
                QuestAlias alias;
                stream >> alias.name;
                qint32 typeVal = 0;
                stream >> typeVal;
                alias.type = typeVal;
                stream >> alias.refId;
                quint8 optVal = 0;
                stream >> optVal;
                alias.optional = (optVal != 0);
                mAliases.append(alias);
            }
        }
    }

    mAliasTable->setRowCount(0);
    for (int i = 0; i < mAliases.size(); ++i)
    {
        int row = mAliasTable->rowCount();
        mAliasTable->insertRow(row);

        auto* nameEdit = new QLineEdit(mAliases[i].name);
        mAliasTable->setCellWidget(row, 0, nameEdit);

        auto* typeCombo = new QComboBox();
        typeCombo->addItem("Reference", 0);
        typeCombo->addItem("Location", 1);
        typeCombo->addItem("Reference Collection", 2);
        typeCombo->addItem("Location Collection", 3);
        typeCombo->addItem("External", 4);
        int typeIdx = typeCombo->findData(mAliases[i].type);
        if (typeIdx >= 0)
            typeCombo->setCurrentIndex(typeIdx);
        mAliasTable->setCellWidget(row, 1, typeCombo);

        auto* refEdit = new QLineEdit(
            QString("0x%1").arg(mAliases[i].refId, 8, 16, QChar('0')).toUpper());
        setFormIdValidator(refEdit, this);
        mAliasTable->setCellWidget(row, 2, refEdit);

        auto* optCheck = new QCheckBox();
        optCheck->setChecked(mAliases[i].optional);
        mAliasTable->setCellWidget(row, 3, optCheck);
    }
}

void QuestAliasEditor::saveToQuest()
{
    mAliases.clear();

    for (int row = 0; row < mAliasTable->rowCount(); ++row)
    {
        auto* nameEdit = qobject_cast<QLineEdit*>(mAliasTable->cellWidget(row, 0));
        auto* typeCombo = qobject_cast<QComboBox*>(mAliasTable->cellWidget(row, 1));
        auto* refEdit = qobject_cast<QLineEdit*>(mAliasTable->cellWidget(row, 2));
        auto* optCheck = qobject_cast<QCheckBox*>(mAliasTable->cellWidget(row, 3));

        if (!nameEdit || !typeCombo || !refEdit || !optCheck)
            continue;

        QuestAlias alias;
        alias.name = nameEdit->text().trimmed();
        alias.type = typeCombo->currentData().toInt();

        QString refText = refEdit->text().trimmed();
        bool ok = false;
        alias.refId = refText.toUInt(&ok, 16);

        alias.optional = optCheck->isChecked();

        mAliases.append(alias);
    }

    for (int i = mQuest->rawSubRecords.size() - 1; i >= 0; --i)
    {
        if (mQuest->rawSubRecords[i].name == 'ALIA')
            mQuest->rawSubRecords.removeAt(i);
    }

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << (quint32)mAliases.size();
    for (const auto& alias : mAliases)
    {
        stream << alias.name;
        stream << (qint32)alias.type;
        stream << alias.refId;
        stream << (quint8)(alias.optional ? 1 : 0);
    }
    mQuest->rawSubRecords.append({'ALIA', data});
}

void QuestAliasEditor::addAlias()
{
    int row = mAliasTable->rowCount();
    mAliasTable->insertRow(row);

    auto* nameEdit = new QLineEdit();
    mAliasTable->setCellWidget(row, 0, nameEdit);

    auto* typeCombo = new QComboBox();
    typeCombo->addItem("Reference", 0);
    typeCombo->addItem("Location", 1);
    typeCombo->addItem("Reference Collection", 2);
    typeCombo->addItem("Location Collection", 3);
    typeCombo->addItem("External", 4);
    mAliasTable->setCellWidget(row, 1, typeCombo);

    auto* refEdit = new QLineEdit("0x00000000");
    setFormIdValidator(refEdit, this);
    mAliasTable->setCellWidget(row, 2, refEdit);

    auto* optCheck = new QCheckBox();
    mAliasTable->setCellWidget(row, 3, optCheck);
}

void QuestAliasEditor::removeAlias()
{
    int row = mAliasTable->currentRow();
    if (row >= 0)
        mAliasTable->removeRow(row);
}

void QuestAliasEditor::saveChanges()
{
    saveToQuest();
    accept();
}
