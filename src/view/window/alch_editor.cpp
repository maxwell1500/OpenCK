#include "alch_editor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "../../../libs/files/esm/alchrecord.hpp"
#include "../../../libs/files/esm/ingrrecord.hpp"
#include "../../../libs/files/esm/magicrecord.hpp"
#include "fieldvalidators.hpp"
#include "logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>
#include <QHeaderView>

AlchEditor::AlchEditor(Data* data, AlchRecord* alch, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mRecord(alch),
      mTabs(nullptr),
      mEditorIdEdit(nullptr),
      mIconPathEdit(nullptr),
      mModelPathEdit(nullptr),
      mEnchantmentSpin(nullptr),
      mWeightSpin(nullptr),
      mValueSpin(nullptr),
      mSynergiesList(nullptr),
      mIngredientTable(nullptr)
{
    mOriginalEditorId = mRecord->editorId;
    setupUI();
    loadFromAlch();
}

QVector<quint32> AlchEditor::extractEffects(const QVector<RawSubRecord>& rawSubRecords)
{
    QVector<quint32> effects;
    for (const auto& raw : rawSubRecords)
    {
        if (raw.name == 'EFID' && raw.data.size() >= static_cast<int>(sizeof(quint32)))
        {
            quint32 formId;
            memcpy(&formId, raw.data.constData(), sizeof(quint32));
            effects.append(formId);
        }
    }
    return effects;
}

QString AlchEditor::schoolName(quint32 school)
{
    switch (school)
    {
        case 0: return QStringLiteral("Alteration");
        case 1: return QStringLiteral("Conjuration");
        case 2: return QStringLiteral("Destruction");
        case 3: return QStringLiteral("Illusion");
        case 4: return QStringLiteral("Mysticism");
        case 5: return QStringLiteral("Restoration");
        default: return QStringLiteral("Unknown");
    }
}

bool AlchEditor::areSchoolsOpposing(quint32 a, quint32 b)
{
    if ((a == 0 && b == 1) || (a == 1 && b == 0)) return true;
    if ((a == 2 && b == 5) || (a == 5 && b == 2)) return true;
    if ((a == 3 && b == 4) || (a == 4 && b == 3)) return true;
    return false;
}

bool AlchEditor::areSchoolsSame(quint32 a, quint32 b)
{
    return a == b;
}

void AlchEditor::setupUI()
{
    setWindowTitle("Alchemy Editor");
    setMinimumSize(520, 440);

    auto* mainLayout = new QVBoxLayout(this);

    mTabs = new QTabWidget();

    auto* generalWidget = new QWidget();
    auto* generalLayout = new QVBoxLayout(generalWidget);

    auto* infoGroup = new QGroupBox("Alchemy Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mIconPathEdit = new QLineEdit();
    infoLayout->addRow("Icon Path:", mIconPathEdit);

    mModelPathEdit = new QLineEdit();
    infoLayout->addRow("Model Path:", mModelPathEdit);

    infoLayout->addRow("", new QLabel("<b>Stats</b>"));

    mEnchantmentSpin = new QSpinBox();
    setIntNonNegativeValidator(mEnchantmentSpin);
    infoLayout->addRow("Enchantment:", mEnchantmentSpin);

    mWeightSpin = new QDoubleSpinBox();
    setWeightValidator(mWeightSpin);
    infoLayout->addRow("Weight:", mWeightSpin);

    mValueSpin = new QSpinBox();
    setValueValidator(mValueSpin);
    infoLayout->addRow("Value:", mValueSpin);

    generalLayout->addWidget(infoGroup);
    generalLayout->addStretch();
    mTabs->addTab(generalWidget, "General");

    auto* synergiesWidget = new QWidget();
    auto* synergiesLayout = new QVBoxLayout(synergiesWidget);

    auto* synergiesGroup = new QGroupBox("Effect Synergies");
    auto* synergiesGroupLayout = new QVBoxLayout(synergiesGroup);

    mSynergiesList = new QListWidget();
    synergiesGroupLayout->addWidget(mSynergiesList);

    synergiesLayout->addWidget(synergiesGroup);
    mTabs->addTab(synergiesWidget, "Synergies");

    auto* ingredientsWidget = new QWidget();
    auto* ingredientsLayout = new QVBoxLayout(ingredientsWidget);

    auto* ingredientsGroup = new QGroupBox("Possible Ingredients");
    auto* ingredientsGroupLayout = new QVBoxLayout(ingredientsGroup);

    mIngredientTable = new QTableWidget();
    mIngredientTable->setColumnCount(3);
    mIngredientTable->setHorizontalHeaderLabels({"Ingredient Name", "Effect Match Count", "Primary Effect"});
    mIngredientTable->horizontalHeader()->setStretchLastSection(true);
    mIngredientTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    mIngredientTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mIngredientTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mIngredientTable->verticalHeader()->setVisible(false);
    ingredientsGroupLayout->addWidget(mIngredientTable);

    ingredientsLayout->addWidget(ingredientsGroup);
    mTabs->addTab(ingredientsWidget, "Ingredients");

    mainLayout->addWidget(mTabs, 1);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &AlchEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void AlchEditor::loadFromAlch()
{
    mEditorIdEdit->setText(mRecord->editorId);
    mIconPathEdit->setText(mRecord->iconPath);
    mModelPathEdit->setText(mRecord->modelPath);
    mWeightSpin->setValue(static_cast<double>(mRecord->weight));
    mValueSpin->setValue(static_cast<int>(mRecord->value));

    buildSynergies();
    buildIngredientTable();
}

void AlchEditor::buildSynergies()
{
    mSynergiesList->clear();

    QVector<quint32> alchEffects = extractEffects(mRecord->rawSubRecords);
    if (alchEffects.size() < 2)
    {
        mSynergiesList->addItem("No synergies to display (less than 2 effects).");
        return;
    }

    const auto& magicCollection = mData->getMagicCollection();
    struct EffectInfo
    {
        quint32 formId;
        QString name;
        quint32 school;
    };

    QVector<EffectInfo> effectInfos;
    for (quint32 formId : alchEffects)
    {
        EffectInfo info;
        info.formId = formId;
        info.name = QString::number(formId, 16);
        info.school = 255;

        for (int i = 0; i < magicCollection.size(); ++i)
        {
            const auto& magic = magicCollection.getRecord(i).get();
            if (magic.formId == formId)
            {
                info.name = magic.editorId;
                info.school = magic.schools;
                break;
            }
        }
        effectInfos.append(info);
    }

    for (int i = 0; i < effectInfos.size(); ++i)
    {
        for (int j = i + 1; j < effectInfos.size(); ++j)
        {
            const auto& a = effectInfos[i];
            const auto& b = effectInfos[j];

            QString label;
            QColor color;

            if (areSchoolsSame(a.school, b.school))
            {
                label = QString("Combined: %1 + %2 = Enhanced (%3 synergy)")
                            .arg(a.name, b.name, schoolName(a.school));
                color = QColor(0, 128, 0);
            }
            else if (areSchoolsOpposing(a.school, b.school))
            {
                label = QString("Opposed: %1 + %2 = Weakened (%3 vs %4)")
                            .arg(a.name, b.name, schoolName(a.school), schoolName(b.school));
                color = QColor(192, 0, 0);
            }
            else if (a.school == 255 || b.school == 255)
            {
                label = QString("Neutral: %1 + %2 (unknown school)")
                            .arg(a.name, b.name);
                color = QColor(128, 128, 128);
            }
            else
            {
                label = QString("Neutral: %1 + %2 (%3 + %4)")
                            .arg(a.name, b.name, schoolName(a.school), schoolName(b.school));
                color = QColor(100, 100, 100);
            }

            auto* item = new QListWidgetItem(label);
            item->setForeground(color);
            mSynergiesList->addItem(item);
        }
    }
}

void AlchEditor::buildIngredientTable()
{
    mIngredientTable->setRowCount(0);

    QVector<quint32> alchEffects = extractEffects(mRecord->rawSubRecords);
    if (alchEffects.isEmpty())
    {
        return;
    }

    const auto& ingrCollection = mData->getIngrCollection();
    const auto& magicCollection = mData->getMagicCollection();

    struct IngredientMatch
    {
        QString name;
        int matchCount;
        QString primaryEffect;
    };
    QVector<IngredientMatch> matches;

    for (int i = 0; i < ingrCollection.size(); ++i)
    {
        const auto& ingr = ingrCollection.getRecord(i).get();
        QVector<quint32> ingrEffects = extractEffects(ingr.rawSubRecords);
        if (ingrEffects.isEmpty())
        {
            continue;
        }

        int matchCount = 0;
        QString primaryEffect;
        bool foundPrimary = false;

        for (quint32 ingrEff : ingrEffects)
        {
            for (quint32 alchEff : alchEffects)
            {
                if (ingrEff == alchEff)
                {
                    matchCount++;
                    if (!foundPrimary)
                    {
                        for (int m = 0; m < magicCollection.size(); ++m)
                        {
                            if (magicCollection.getRecord(m).get().formId == ingrEff)
                            {
                                primaryEffect = magicCollection.getRecord(m).get().editorId;
                                foundPrimary = true;
                                break;
                            }
                        }
                        if (!foundPrimary)
                        {
                            primaryEffect = QString::number(ingrEff, 16);
                            foundPrimary = true;
                        }
                    }
                }
            }
        }

        if (matchCount > 0)
        {
            matches.append({ingr.editorId, matchCount, primaryEffect});
        }
    }

    std::sort(matches.begin(), matches.end(), [](const IngredientMatch& a, const IngredientMatch& b) {
        return a.matchCount > b.matchCount;
    });

    int maxMatches = matches.isEmpty() ? 0 : matches.first().matchCount;

    mIngredientTable->setRowCount(matches.size());
    for (int row = 0; row < matches.size(); ++row)
    {
        const auto& match = matches[row];

        auto* nameItem = new QTableWidgetItem(match.name);
        auto* countItem = new QTableWidgetItem(QString::number(match.matchCount));
        auto* effectItem = new QTableWidgetItem(match.primaryEffect);

        if (match.matchCount > 1 && maxMatches > 0)
        {
            int intensity = static_cast<int>(255.0 * match.matchCount / maxMatches);
            QColor bg(180 + intensity / 3, 220 + intensity / 7, 180 + intensity / 3);
            nameItem->setBackground(bg);
            countItem->setBackground(bg);
            effectItem->setBackground(bg);
        }

        mIngredientTable->setItem(row, 0, nameItem);
        mIngredientTable->setItem(row, 1, countItem);
        mIngredientTable->setItem(row, 2, effectItem);
    }
}

bool AlchEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getAlchCollection().searchId(editorId) >= 0)
    {
        if (editorId != mRecord->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "An alchemy item with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void AlchEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    {
        auto results = ColumnValidator::validateAlch(*mRecord, mData, mOriginalEditorId);
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

    mRecord->editorId = mEditorIdEdit->text();
    mRecord->iconPath = mIconPathEdit->text();
    mRecord->modelPath = mModelPathEdit->text();
    mRecord->weight = static_cast<float>(mWeightSpin->value());
    mRecord->value = static_cast<quint32>(mValueSpin->value());

    accept();
}
