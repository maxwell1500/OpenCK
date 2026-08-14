#include "npceditor.hpp"
#include "fieldvalidators.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "logger.hpp"
#include "npcrecord.hpp"
#include "records.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QGroupBox>
#include <QScrollBar>
#include <QMessageBox>
#include <QDataStream>
#include <QInputDialog>

static constexpr NAME makeName(const char s[4])
{
    return (static_cast<quint32>(static_cast<quint8>(s[0])) |
            static_cast<quint32>(static_cast<quint8>(s[1])) << 8 |
            static_cast<quint32>(static_cast<quint8>(s[2])) << 16 |
            static_cast<quint32>(static_cast<quint8>(s[3])) << 24);
}

static const NAME sHdpt = makeName("HDPT");
static const NAME sSknt = makeName("SKNT");
static const NAME sWght = makeName("WGHT");
static const NAME sHght = makeName("HGHT");
static const NAME sVoic = makeName("VOIC");
static const NAME sLvld = makeName("LVLD");

static QVector<RawSubRecord>::iterator findSubRecord(QVector<RawSubRecord>& records, NAME name)
{
    for (auto it = records.begin(); it != records.end(); ++it)
    {
        if (it->name == name)
            return it;
    }
    return records.end();
}

static QVector<RawSubRecord>::const_iterator findSubRecord(const QVector<RawSubRecord>& records, NAME name)
{
    for (auto it = records.begin(); it != records.end(); ++it)
    {
        if (it->name == name)
            return it;
    }
    return records.end();
}

static quint32 parseHexFormId(const QString& text, bool* ok)
{
    QString clean = text.trimmed();
    if (clean.startsWith("0x", Qt::CaseInsensitive))
        clean = clean.mid(2);
    return clean.trimmed().toUInt(ok, 16);
}

NpcEditor::NpcEditor(Data* data, NpcRecord* npc, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mRecord(npc),
      mTabWidget(nullptr),
      mEditorIdEdit(nullptr),
      mFullNameEdit(nullptr),
      mLevelSpin(nullptr),
      mRaceCombo(nullptr),
      mSexCombo(nullptr),
      mClassCombo(nullptr),
      mFactionCombo(nullptr),
      mHealthSpin(nullptr),
      mMagickaSpin(nullptr),
      mStaminaSpin(nullptr),
      mAggroRadiusSpin(nullptr),
      mCombatSpin(nullptr),
      mSpellsList(nullptr),
      mInventoryTable(nullptr),
      mRelationshipsList(nullptr),
      mHeadPartHead(nullptr),
      mHeadPartEyes(nullptr),
      mHeadPartBrows(nullptr),
      mHeadPartNose(nullptr),
      mHeadPartMouth(nullptr),
      mHeadPartHair(nullptr),
      mHeadPartBeard(nullptr),
      mHeadPartScar(nullptr),
      mSkinToneCombo(nullptr),
      mWeightSlider(nullptr),
      mWeightSpin(nullptr),
      mHeightSlider(nullptr),
      mHeightSpin(nullptr),
      mVoiceTypeCombo(nullptr),
      mIsLeveledCheck(nullptr),
      mLeveledGroup(nullptr),
      mMinLevelSpin(nullptr),
      mMaxLevelSpin(nullptr),
      mLeveledTemplateEdit(nullptr),
      mLevelMultiplierSpin(nullptr)
{
    LOG_INFO(QString("NpcEditor created for '%1'").arg(mRecord->editorId));
    mOriginalEditorId = mRecord->editorId;
    setupUI();
    loadFromNpc();
}

NpcEditor::~NpcEditor()
{
}

void NpcEditor::setupUI()
{
    setWindowTitle("NPC Editor");
    setMinimumSize(500, 600);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    mTabWidget = new QTabWidget();

    // General Tab
    {
        auto* generalTab = new QWidget();
        auto* layout = new QFormLayout(generalTab);
        layout->setContentsMargins(10, 5, 10, 5);

        mEditorIdEdit = new QLineEdit();
        mEditorIdEdit->setPlaceholderText("Enter Editor ID...");
        mEditorIdEdit->setReadOnly(true);
        layout->addRow("Editor ID:", mEditorIdEdit);

        mFullNameEdit = new QLineEdit();
        mFullNameEdit->setPlaceholderText("Enter full name...");
        layout->addRow("Full Name:", mFullNameEdit);

        mLevelSpin = new QSpinBox();
        mLevelSpin->setRange(0, 65535);
        layout->addRow("Level:", mLevelSpin);

        mRaceCombo = new QComboBox();
        layout->addRow("Race:", mRaceCombo);

        mSexCombo = new QComboBox();
        mSexCombo->addItem("Male");
        mSexCombo->addItem("Female");
        layout->addRow("Sex:", mSexCombo);

        mClassCombo = new QComboBox();
        layout->addRow("Class:", mClassCombo);

        mFactionCombo = new QComboBox();
        layout->addRow("Faction:", mFactionCombo);

        mIsLeveledCheck = new QCheckBox("Is Leveled NPC");
        layout->addRow(QString(), mIsLeveledCheck);

        mLeveledGroup = new QGroupBox("Leveled Settings");
        auto* leveledLayout = new QFormLayout(mLeveledGroup);

        mMinLevelSpin = new QSpinBox();
        mMinLevelSpin->setRange(0, 65535);
        leveledLayout->addRow("Min Level:", mMinLevelSpin);

        mMaxLevelSpin = new QSpinBox();
        mMaxLevelSpin->setRange(0, 65535);
        leveledLayout->addRow("Max Level:", mMaxLevelSpin);

        mLeveledTemplateEdit = new QLineEdit();
        mLeveledTemplateEdit->setPlaceholderText("Template FormID...");
        setHexFormIdValidator(mLeveledTemplateEdit, this);
        leveledLayout->addRow("Leveled Template:", mLeveledTemplateEdit);

        mLevelMultiplierSpin = new QDoubleSpinBox();
        mLevelMultiplierSpin->setRange(0.0, 10.0);
        mLevelMultiplierSpin->setDecimals(2);
        mLevelMultiplierSpin->setSingleStep(0.1);
        leveledLayout->addRow("Level Multiplier:", mLevelMultiplierSpin);

        layout->addRow(mLeveledGroup);

        connect(mIsLeveledCheck, &QCheckBox::toggled, mLeveledGroup, &QGroupBox::setVisible);

        mTabWidget->addTab(generalTab, "General");
    }

    // Appearance Tab
    {
        auto* appearanceTab = new QWidget();
        auto* layout = new QFormLayout(appearanceTab);
        layout->setContentsMargins(10, 5, 10, 5);

        mHeadPartHead = new QLineEdit();
        mHeadPartHead->setPlaceholderText("Head FormID...");
        setHexFormIdValidator(mHeadPartHead, this);
        layout->addRow("Head:", mHeadPartHead);

        mHeadPartEyes = new QLineEdit();
        mHeadPartEyes->setPlaceholderText("Eyes FormID...");
        setHexFormIdValidator(mHeadPartEyes, this);
        layout->addRow("Eyes:", mHeadPartEyes);

        mHeadPartBrows = new QLineEdit();
        mHeadPartBrows->setPlaceholderText("Brows FormID...");
        setHexFormIdValidator(mHeadPartBrows, this);
        layout->addRow("Brows:", mHeadPartBrows);

        mHeadPartNose = new QLineEdit();
        mHeadPartNose->setPlaceholderText("Nose FormID...");
        setHexFormIdValidator(mHeadPartNose, this);
        layout->addRow("Nose:", mHeadPartNose);

        mHeadPartMouth = new QLineEdit();
        mHeadPartMouth->setPlaceholderText("Mouth FormID...");
        setHexFormIdValidator(mHeadPartMouth, this);
        layout->addRow("Mouth:", mHeadPartMouth);

        mHeadPartHair = new QLineEdit();
        mHeadPartHair->setPlaceholderText("Hair FormID...");
        setHexFormIdValidator(mHeadPartHair, this);
        layout->addRow("Hair:", mHeadPartHair);

        mHeadPartBeard = new QLineEdit();
        mHeadPartBeard->setPlaceholderText("Beard FormID...");
        setHexFormIdValidator(mHeadPartBeard, this);
        layout->addRow("Beard:", mHeadPartBeard);

        mHeadPartScar = new QLineEdit();
        mHeadPartScar->setPlaceholderText("Scar FormID...");
        setHexFormIdValidator(mHeadPartScar, this);
        layout->addRow("Scar:", mHeadPartScar);

        mSkinToneCombo = new QComboBox();
        mSkinToneCombo->addItems({"Pale", "Fair", "Tan", "Dark", "Redguard Fair", "Redguard Dark"});
        layout->addRow("Skin Tone:", mSkinToneCombo);

        auto* weightRow = new QHBoxLayout();
        mWeightSlider = new QSlider(Qt::Horizontal);
        mWeightSlider->setRange(0, 100);
        mWeightSpin = new QSpinBox();
        mWeightSpin->setRange(0, 100);
        mWeightSpin->setSuffix("%");
        weightRow->addWidget(mWeightSlider);
        weightRow->addWidget(mWeightSpin);
        layout->addRow("Weight:", weightRow);

        connect(mWeightSlider, &QSlider::valueChanged, mWeightSpin, &QSpinBox::setValue);
        connect(mWeightSpin, QOverload<int>::of(&QSpinBox::valueChanged), mWeightSlider, &QSlider::setValue);

        auto* heightRow = new QHBoxLayout();
        mHeightSlider = new QSlider(Qt::Horizontal);
        mHeightSlider->setRange(80, 120);
        mHeightSpin = new QSpinBox();
        mHeightSpin->setRange(80, 120);
        mHeightSpin->setSuffix("%");
        heightRow->addWidget(mHeightSlider);
        heightRow->addWidget(mHeightSpin);
        layout->addRow("Height:", heightRow);

        connect(mHeightSlider, &QSlider::valueChanged, mHeightSpin, &QSpinBox::setValue);
        connect(mHeightSpin, QOverload<int>::of(&QSpinBox::valueChanged), mHeightSlider, &QSlider::setValue);

        mVoiceTypeCombo = new QComboBox();
        mVoiceTypeCombo->setEditable(true);
        mVoiceTypeCombo->addItems({
            "MaleNord", "FemaleNord",
            "MaleElf", "FemaleElf",
            "MaleOrc", "FemaleOrc",
            "MaleKhajiit", "FemaleKhajiit",
            "MaleArgonian", "FemaleArgonian",
            "MaleCommoner", "FemaleCommoner",
            "MaleGuard", "FemaleGuard",
            "MaleChild", "FemaleChild"
        });
        layout->addRow("Voice Type:", mVoiceTypeCombo);

        mTabWidget->addTab(appearanceTab, "Appearance");
    }

    // Stats Tab
    {
        auto* statsTab = new QWidget();
        auto* layout = new QFormLayout(statsTab);
        layout->setContentsMargins(10, 5, 10, 5);

        mHealthSpin = new QSpinBox();
        mHealthSpin->setRange(0, 65535);
        layout->addRow("Health:", mHealthSpin);

        mMagickaSpin = new QSpinBox();
        mMagickaSpin->setRange(0, 65535);
        layout->addRow("Magicka:", mMagickaSpin);

        mStaminaSpin = new QSpinBox();
        mStaminaSpin->setRange(0, 65535);
        layout->addRow("Stamina:", mStaminaSpin);

        mTabWidget->addTab(statsTab, "Stats");
    }

    // AI Tab
    {
        auto* aiTab = new QWidget();
        auto* layout = new QFormLayout(aiTab);
        layout->setContentsMargins(10, 5, 10, 5);

        mAggroRadiusSpin = new QSpinBox();
        mAggroRadiusSpin->setRange(0, 65535);
        layout->addRow("Aggro Radius:", mAggroRadiusSpin);

        mCombatSpin = new QSpinBox();
        mCombatSpin->setRange(0, 65535);
        layout->addRow("Combat:", mCombatSpin);

        mTabWidget->addTab(aiTab, "AI");
    }

    // Spells Tab
    {
        auto* spellsTab = new QWidget();
        auto* layout = new QVBoxLayout(spellsTab);
        layout->setContentsMargins(5, 5, 5, 5);

        mSpellsList = new QListWidget();
        mSpellsList->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        layout->addWidget(mSpellsList, 1);

        auto* buttonRow = new QHBoxLayout();
        auto* addButton = new QPushButton("Add...");
        auto* removeButton = new QPushButton("Remove");
        buttonRow->addStretch();
        buttonRow->addWidget(addButton);
        buttonRow->addWidget(removeButton);
        layout->addLayout(buttonRow);

        connect(addButton, &QPushButton::clicked, this, [this]() {
            bool ok = false;
            QString text = QInputDialog::getText(this, "Add Spell",
                "Enter the spell's Form ID in hex (e.g. 0x0001A2B3):", QLineEdit::Normal, "0x", &ok);
            if (!ok) return;
            bool parsed = false;
            quint32 val = parseHexFormId(text, &parsed);
            if (!parsed || val == 0) return;
            addSpellRow(QString::number(val));
        });
        connect(removeButton, &QPushButton::clicked, this, [this]() {
            delete mSpellsList->currentItem();
        });

        mTabWidget->addTab(spellsTab, "Spells");
    }

    // Inventory Tab (Equipment/Items)
    {
        auto* invTab = new QWidget();
        auto* layout = new QVBoxLayout(invTab);
        layout->setContentsMargins(5, 5, 5, 5);

        mInventoryTable = new QTableWidget(0, 5);
        mInventoryTable->setHorizontalHeaderLabels({"FormID", "Count", "Equipped", "Slot", ""});
        mInventoryTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        mInventoryTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        mInventoryTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        mInventoryTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        mInventoryTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
        mInventoryTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        layout->addWidget(mInventoryTable, 1);

        auto* buttonRow = new QHBoxLayout();
        auto* addButton = new QPushButton("Add Item");
        auto* removeButton = new QPushButton("Remove Selected");
        buttonRow->addStretch();
        buttonRow->addWidget(addButton);
        buttonRow->addWidget(removeButton);
        layout->addLayout(buttonRow);

        connect(addButton, &QPushButton::clicked, this, [this]() {
            bool ok = false;
            QString text = QInputDialog::getText(this, "Add Item",
                "Enter the item's Form ID in hex (e.g. 0x0001A2B3):", QLineEdit::Normal, "0x", &ok);
            if (!ok) return;
            bool parsed = false;
            quint32 val = parseHexFormId(text, &parsed);
            if (!parsed || val == 0) return;
            addInventoryRow(QString::number(val), "1", "None", false);
        });
        connect(removeButton, &QPushButton::clicked, this, [this]() {
            int row = mInventoryTable->currentRow();
            if (row >= 0) mInventoryTable->removeRow(row);
        });

        mTabWidget->addTab(invTab, "Equipment");
    }

    // Relationships Tab
    {
        auto* relTab = new QWidget();
        auto* layout = new QVBoxLayout(relTab);
        layout->setContentsMargins(5, 5, 5, 5);

        mRelationshipsList = new QListWidget();
        mRelationshipsList->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        layout->addWidget(mRelationshipsList, 1);

        auto* buttonRow = new QHBoxLayout();
        auto* addButton = new QPushButton("Add...");
        auto* removeButton = new QPushButton("Remove");
        buttonRow->addStretch();
        buttonRow->addWidget(addButton);
        buttonRow->addWidget(removeButton);
        layout->addLayout(buttonRow);

        connect(addButton, &QPushButton::clicked, this, [this]() {
            bool ok = false;
            QString text = QInputDialog::getText(this, "Add Relationship",
                "Enter the faction's Form ID in hex (e.g. 0x0001A2B3):", QLineEdit::Normal, "0x", &ok);
            if (!ok) return;
            bool parsed = false;
            quint32 val = parseHexFormId(text, &parsed);
            if (!parsed || val == 0) return;
            addRelationshipRow(QString::number(val));
        });
        connect(removeButton, &QPushButton::clicked, this, [this]() {
            delete mRelationshipsList->currentItem();
        });

        mTabWidget->addTab(relTab, "Relationships");
    }

    mainLayout->addWidget(mTabWidget, 1);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveButton = new QPushButton("Save");
    saveButton->setToolTip("Save changes to the NPC record");
    buttonLayout->addWidget(saveButton);

    auto* cancelButton = new QPushButton("Cancel");
    cancelButton->setToolTip("Close without saving");
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    connect(saveButton, &QPushButton::clicked, this, &NpcEditor::saveChanges);
    connect(cancelButton, &QPushButton::clicked, this, &NpcEditor::cancelEdit);
}

void NpcEditor::loadFromNpc()
{
    mEditorIdEdit->setText(mRecord->editorId);
    mFullNameEdit->setText(mRecord->fullName);
    mLevelSpin->setValue(mRecord->level);
    mHealthSpin->setValue(mRecord->health);
    mMagickaSpin->setValue(mRecord->magicka);
    mStaminaSpin->setValue(mRecord->stamina);
    mAggroRadiusSpin->setValue(mRecord->aiAggroRadius);
    mCombatSpin->setValue(mRecord->aiCombat);

    mSexCombo->setCurrentIndex(mRecord->sex == 1 ? 1 : 0);

    mRaceCombo->setEditText(QString::number(mRecord->race));
    mClassCombo->setEditText(QString::number(mRecord->class_));
    mFactionCombo->setEditText(QString::number(mRecord->faction));

    mSpellsList->clear();
    for (auto spell : mRecord->spells)
    {
        addSpellRow(QString::number(spell));
    }

    mInventoryTable->setRowCount(0);
    for (auto item : mRecord->inventoryItems)
    {
        addInventoryRow(QString::number(item), "1", "None", false);
    }

    mRelationshipsList->clear();
    for (auto rel : mRecord->relationships)
    {
        addRelationshipRow(QString::number(rel));
    }

    {
        auto it = findSubRecord(mRecord->rawSubRecords, sHdpt);
        if (it != mRecord->rawSubRecords.end())
        {
            QDataStream stream(it->data);
            stream.setByteOrder(QDataStream::LittleEndian);
            quint32 count = 0;
            stream >> count;
            QLineEdit* headParts[8] = {
                mHeadPartHead, mHeadPartEyes, mHeadPartBrows, mHeadPartNose,
                mHeadPartMouth, mHeadPartHair, mHeadPartBeard, mHeadPartScar
            };
            for (quint32 i = 0; i < 8 && i < count; ++i)
            {
                quint32 formId = 0;
                stream >> formId;
                if (formId != 0)
                    headParts[i]->setText(QString::number(formId));
            }
        }
    }

    {
        auto it = findSubRecord(mRecord->rawSubRecords, sSknt);
        if (it != mRecord->rawSubRecords.end())
        {
            QDataStream stream(it->data);
            stream.setByteOrder(QDataStream::LittleEndian);
            quint32 index = 0;
            stream >> index;
            if (index < static_cast<quint32>(mSkinToneCombo->count()))
                mSkinToneCombo->setCurrentIndex(static_cast<int>(index));
        }
    }

    {
        auto it = findSubRecord(mRecord->rawSubRecords, sWght);
        if (it != mRecord->rawSubRecords.end())
        {
            QDataStream stream(it->data);
            stream.setByteOrder(QDataStream::LittleEndian);
            float weight = 0.0f;
            stream >> weight;
            mWeightSlider->setValue(static_cast<int>(weight));
        }
    }

    {
        auto it = findSubRecord(mRecord->rawSubRecords, sHght);
        if (it != mRecord->rawSubRecords.end())
        {
            QDataStream stream(it->data);
            stream.setByteOrder(QDataStream::LittleEndian);
            float height = 1.0f;
            stream >> height;
            mHeightSlider->setValue(static_cast<int>(height * 100.0f));
        }
    }

    {
        auto it = findSubRecord(mRecord->rawSubRecords, sVoic);
        if (it != mRecord->rawSubRecords.end())
        {
            QDataStream stream(it->data);
            stream.setByteOrder(QDataStream::LittleEndian);
            quint32 formId = 0;
            stream >> formId;
            mVoiceTypeCombo->setEditText(QString::number(formId));
        }
    }

    {
        auto it = findSubRecord(mRecord->rawSubRecords, sLvld);
        if (it != mRecord->rawSubRecords.end())
        {
            QDataStream stream(it->data);
            stream.setByteOrder(QDataStream::LittleEndian);
            quint32 flags = 0;
            quint32 minLevel = 0;
            quint32 maxLevel = 0;
            quint32 templateId = 0;
            float multiplier = 1.0f;
            stream >> flags >> minLevel >> maxLevel >> templateId >> multiplier;

            bool isLeveled = (flags & 1) != 0;
            mIsLeveledCheck->setChecked(isLeveled);
            mLeveledGroup->setVisible(isLeveled);
            mMinLevelSpin->setValue(static_cast<int>(minLevel));
            mMaxLevelSpin->setValue(static_cast<int>(maxLevel));
            if (templateId != 0)
                mLeveledTemplateEdit->setText(QString::number(templateId));
            mLevelMultiplierSpin->setValue(static_cast<double>(multiplier));
        }
        else
        {
            mLeveledGroup->setVisible(false);
        }
    }
}

void NpcEditor::saveToNpc()
{
    mRecord->editorId = mEditorIdEdit->text();
    mRecord->fullName = mFullNameEdit->text();
    mRecord->level = mLevelSpin->value();
    mRecord->health = mHealthSpin->value();
    mRecord->magicka = mMagickaSpin->value();
    mRecord->stamina = mStaminaSpin->value();
    mRecord->aiAggroRadius = mAggroRadiusSpin->value();
    mRecord->aiCombat = mCombatSpin->value();
    mRecord->sex = mSexCombo->currentIndex();

    bool ok = false;
    mRecord->race = mRaceCombo->currentText().toUInt(&ok);
    if (!ok) mRecord->race = 0;

    mRecord->class_ = mClassCombo->currentText().toUInt(&ok);
    if (!ok) mRecord->class_ = 0;

    mRecord->faction = mFactionCombo->currentText().toUInt(&ok);
    if (!ok) mRecord->faction = 0;

    mRecord->spells.clear();
    for (int i = 0; i < mSpellsList->count(); i++)
    {
        QString text = mSpellsList->item(i)->text();
        bool ok2 = false;
        quint32 val = text.toUInt(&ok2);
        if (ok2)
        {
            mRecord->spells.push_back(val);
        }
    }

    mRecord->inventoryItems.clear();
    QStringList skippedInventory;
    for (int i = 0; i < mInventoryTable->rowCount(); i++)
    {
        QTableWidgetItem* formItem = mInventoryTable->item(i, 0);
        QString text = formItem ? formItem->text() : QString();
        bool ok = false;
        quint32 val = text.toUInt(&ok);
        if (ok && val != 0) {
            mRecord->inventoryItems.push_back(val);
        } else {
            skippedInventory << QString("%1: '%2'").arg(i + 1).arg(text);
        }
    }
    if (!skippedInventory.isEmpty())
    {
        QMessageBox::warning(this, "Invalid Inventory Items",
            QString("The following inventory rows were skipped because their Form ID is not a valid number:\n%1")
                .arg(skippedInventory.join("\n")));
    }

    mRecord->relationships.clear();
    for (int i = 0; i < mRelationshipsList->count(); i++)
    {
        QString text = mRelationshipsList->item(i)->text();
        bool ok2 = false;
        quint32 val = text.toUInt(&ok2);
        if (ok2)
        {
            mRecord->relationships.push_back(val);
        }
    }

    auto removeSubRecord = [this](NAME name) {
        auto it = findSubRecord(mRecord->rawSubRecords, name);
        if (it != mRecord->rawSubRecords.end())
            mRecord->rawSubRecords.erase(it);
    };

    auto addOrReplace = [this](NAME name, const QByteArray& data) {
        auto it = findSubRecord(mRecord->rawSubRecords, name);
        if (it != mRecord->rawSubRecords.end())
            it->data = data;
        else
            mRecord->rawSubRecords.push_back({name, data});
    };

    {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << static_cast<quint32>(8);

        QLineEdit* headParts[8] = {
            mHeadPartHead, mHeadPartEyes, mHeadPartBrows, mHeadPartNose,
            mHeadPartMouth, mHeadPartHair, mHeadPartBeard, mHeadPartScar
        };
        for (int i = 0; i < 8; ++i)
        {
            bool ok = false;
            quint32 formId = headParts[i]->text().toUInt(&ok);
            stream << (ok ? formId : quint32(0));
        }
        addOrReplace(sHdpt, data);
    }

    {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << static_cast<quint32>(mSkinToneCombo->currentIndex());
        addOrReplace(sSknt, data);
    }

    {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << static_cast<float>(mWeightSlider->value());
        addOrReplace(sWght, data);
    }

    {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << static_cast<float>(mHeightSlider->value() / 100.0f);
        addOrReplace(sHght, data);
    }

    {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        bool ok = false;
        quint32 formId = mVoiceTypeCombo->currentText().toUInt(&ok);
        stream << (ok ? formId : quint32(0));
        addOrReplace(sVoic, data);
    }

    {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        quint32 flags = mIsLeveledCheck->isChecked() ? 1 : 0;
        stream << flags
               << static_cast<quint32>(mMinLevelSpin->value())
               << static_cast<quint32>(mMaxLevelSpin->value());

        bool ok = false;
        quint32 templateId = mLeveledTemplateEdit->text().toUInt(&ok);
        stream << (ok ? templateId : quint32(0));

        stream << static_cast<float>(mLevelMultiplierSpin->value());
        addOrReplace(sLvld, data);
    }
}

void NpcEditor::saveChanges()
{
    if (!validateNpc())
    {
        return;
    }

    {
        auto results = ColumnValidator::validateNpc(*mRecord, mData, mOriginalEditorId);
        QStringList errorMessages;
        for (const auto& r : results) {
            if (r.severity == ColumnValidator::Severity::Error) {
                errorMessages << QString("%1: %2").arg(r.field, r.message);
            }
        }
        if (!errorMessages.isEmpty()) {
            QMessageBox::warning(this, tr("Validation Errors"), errorMessages.join("\n"));
            return;
        }
    }

    saveToNpc();

    LOG_INFO(QString("NPC Editor saved successfully, %1 spells, %2 items, %3 relationships")
        .arg(mRecord->spells.size()).arg(mRecord->inventoryItems.size()).arg(mRecord->relationships.size()));
    accept();
}

bool NpcEditor::validateNpc()
{
    if (mEditorIdEdit->text().isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "NPC must have an EditorID.");
        return false;
    }

    auto& collection = mData->getNpcCollection();
    int idx = collection.searchId(mEditorIdEdit->text());
    if (idx >= 0 && collection.getRecord(idx).get().formId != mRecord->formId)
    {
        QMessageBox::warning(this, "Validation Error", 
            QString("EditorID '%1' already exists on another NPC.").arg(mEditorIdEdit->text()));
        return false;
    }

    quint32 race = 0;
    {
        bool ok = false;
        race = mRaceCombo->currentText().toUInt(&ok);
        if (!ok || race == 0)
        {
            QMessageBox::warning(this, "Validation Error", "Race field is required.");
            return false;
        }
    }

    quint32 faction = 0;
    {
        bool ok = false;
        faction = mFactionCombo->currentText().toUInt(&ok);
        if (!ok) faction = 0;
    }
    if (faction == 0)
    {
        QMessageBox::warning(this, "Validation Error", "Faction field is required.");
        return false;
    }

    int level = mLevelSpin->value();
    if (level > 9999)
    {
        QMessageBox::warning(this, "Validation Error", 
            QString("Level value %1 is invalid. Must be 0-9999.").arg(level));
        return false;
    }

    for (int s = 0; s < mRecord->spells.size(); ++s)
    {
        if (mRecord->spells[s] == 0)
        {
            QMessageBox::warning(this, "Validation Error", 
                QString("Spell list at index %1 has an invalid formID (0).").arg(s));
            return false;
        }
    }

    for (int inv = 0; inv < mRecord->inventoryItems.size(); ++inv)
    {
        if (mRecord->inventoryItems[inv] == 0)
        {
            QMessageBox::warning(this, "Validation Error", 
                QString("Inventory item at index %1 has an invalid formID (0).").arg(inv));
            return false;
        }
    }

    return true;
}

void NpcEditor::cancelEdit()
{
    reject();
}

void NpcEditor::addSpellRow(const QString& spellId)
{
    mSpellsList->addItem(spellId);
}

void NpcEditor::addInventoryRow(const QString& formId, const QString& count, const QString& slot, bool equipped)
{
    int row = mInventoryTable->rowCount();
    mInventoryTable->insertRow(row);
    
    auto* formItem = new QTableWidgetItem(formId);
    mInventoryTable->setItem(row, 0, formItem);
    
    auto* countItem = new QTableWidgetItem(count);
    countItem->setFlags(countItem->flags() & ~Qt::ItemIsEditable);
    countItem->setToolTip("not persisted");
    mInventoryTable->setItem(row, 1, countItem);
    
    auto* equippedWidget = new QWidget();
    auto* equippedLayout = new QHBoxLayout(equippedWidget);
    equippedLayout->setContentsMargins(4, 0, 4, 0);
    equippedLayout->setAlignment(Qt::AlignCenter);
    auto* equippedCheck = new QCheckBox();
    equippedCheck->setChecked(equipped);
    equippedCheck->setEnabled(false);
    equippedCheck->setToolTip("not persisted");
    equippedLayout->addWidget(equippedCheck);
    mInventoryTable->setCellWidget(row, 2, equippedWidget);
    
    auto* slotCombo = new QComboBox();
    slotCombo->addItems({"None", "Head", "Hair", "Body", "Hands", "Forearms",
                         "Amulet", "Ring", "Feet", "Calves", "Shield", "Tail",
                         "LongHair", "Circlet", "Ears", "Weapon", "Ammo"});
    slotCombo->setCurrentText(slot);
    slotCombo->setEnabled(false);
    slotCombo->setToolTip("not persisted");
    mInventoryTable->setCellWidget(row, 3, slotCombo);
    
    auto* removeBtn = new QPushButton("X");
    removeBtn->setFixedSize(24, 24);
    connect(removeBtn, &QPushButton::clicked, this, [this, removeBtn]() {
        for (int r = 0; r < mInventoryTable->rowCount(); ++r) {
            if (mInventoryTable->cellWidget(r, 4) == removeBtn) {
                mInventoryTable->removeRow(r);
                break;
            }
        }
    });
    mInventoryTable->setCellWidget(row, 4, removeBtn);
}

void NpcEditor::addRelationshipRow(const QString& factionId)
{
    mRelationshipsList->addItem(factionId);
}
