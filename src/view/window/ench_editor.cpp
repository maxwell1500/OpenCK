#include "ench_editor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "Enchrecord.hpp"
#include "nifviewportwidget.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>
#include <QDialog>

EnchEditor::EnchEditor(Data* data, EnchRecord* ench, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mRecord(ench),
      mEditorIdEdit(nullptr),
      mNameEdit(nullptr),
      mCostLimitSpin(nullptr),
      mChargesSpin(nullptr),
      mEnchantmentDataSpin(nullptr),
      mChargeSpin(nullptr),
      mDurationSpin(nullptr),
      mMagnitudeSpin(nullptr),
      mTypeCombo(nullptr),
      mSoulGemCombo(nullptr),
      mPreviewTypeLabel(nullptr),
      mPreviewInfoLabel(nullptr),
      mPreviewBtn(nullptr)
{
    mOriginalEditorId = mRecord->editorId;
    setupUI();
    loadFromEnch();
}

void EnchEditor::setupUI()
{
    setWindowTitle("Enchantment Editor");
    setMinimumSize(450, 400);

    auto* mainLayout = new QVBoxLayout(this);
    auto* tabs = new QTabWidget();

    // General Tab
    auto* infoGroup = new QGroupBox("Enchantment Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mNameEdit = new QLineEdit();
    infoLayout->addRow("Name:", mNameEdit);

    infoLayout->addRow("", new QLabel("<b>Stats</b>"));

    mCostLimitSpin = new QSpinBox();
    mCostLimitSpin->setRange(0, 99999);
    infoLayout->addRow("Cost Limit:", mCostLimitSpin);

    mChargesSpin = new QSpinBox();
    mChargesSpin->setRange(0, 9999);
    infoLayout->addRow("Charges:", mChargesSpin);

    mEnchantmentDataSpin = new QSpinBox();
    mEnchantmentDataSpin->setRange(0, 9999);
    infoLayout->addRow("Enchantment Data:", mEnchantmentDataSpin);

    mChargeSpin = new QDoubleSpinBox();
    mChargeSpin->setRange(0.0, 99999.0);
    mChargeSpin->setDecimals(1);
    infoLayout->addRow("Charge:", mChargeSpin);

    mDurationSpin = new QSpinBox();
    mDurationSpin->setRange(0, 999999);
    infoLayout->addRow("Duration:", mDurationSpin);

    mMagnitudeSpin = new QDoubleSpinBox();
    mMagnitudeSpin->setRange(0.0, 9999.0);
    mMagnitudeSpin->setDecimals(1);
    infoLayout->addRow("Magnitude:", mMagnitudeSpin);

    tabs->addTab(infoGroup, "General");

    // Effects Tab
    auto* effectsGroup = new QGroupBox("Effect Settings");
    auto* effectsLayout = new QFormLayout(effectsGroup);

    mTypeCombo = new QComboBox();
    mTypeCombo->addItems({
        "Fire Damage",
        "Frost Damage",
        "Shock Damage",
        "Absorb Health",
        "Absorb Magicka",
        "Absorb Stamina",
        "Fortify Skill",
        "Fortify Attribute",
        "Resist Damage",
        "Resist Magic",
        "Resist Disease",
        "Resist Poison",
        "Waterbreathing",
        "Waterwalking",
        "Muffle",
        "Invisibility",
        "Paralysis",
        "Soul Trap"
    });
    effectsLayout->addRow("Enchantment Type:", mTypeCombo);

    mSoulGemCombo = new QComboBox();
    mSoulGemCombo->addItems({
        "Petty",
        "Lesser",
        "Common",
        "Greater",
        "Grand",
        "Black"
    });
    effectsLayout->addRow("Soul Gem:", mSoulGemCombo);

    auto* previewGroup = new QGroupBox("Effect Preview");
    auto* previewLayout = new QVBoxLayout(previewGroup);

    mPreviewTypeLabel = new QLabel("Enchantment type preview");
    mPreviewTypeLabel->setStyleSheet("font-weight: bold;");
    previewLayout->addWidget(mPreviewTypeLabel);

    mPreviewInfoLabel = new QLabel("");
    previewLayout->addWidget(mPreviewInfoLabel);

    mPreviewViewport = new NifViewportWidget(previewGroup);
    mPreviewViewport->setMinimumHeight(200);
    previewLayout->addWidget(mPreviewViewport, 1);

    mPreviewBtn = new QPushButton("Preview Enchantment (Full Size)");
    previewLayout->addWidget(mPreviewBtn);

    connect(mPreviewBtn, &QPushButton::clicked, this, &EnchEditor::previewEnchantment);

    auto* effectsTabWidget = new QWidget();
    auto* effectsTabLayout = new QVBoxLayout(effectsTabWidget);
    effectsTabLayout->setContentsMargins(0, 0, 0, 0);
    effectsTabLayout->addWidget(effectsGroup);
    effectsTabLayout->addWidget(previewGroup);

    tabs->addTab(effectsTabWidget, "Effects");

    mainLayout->addWidget(tabs, 1);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &EnchEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void EnchEditor::loadFromEnch()
{
    mEditorIdEdit->setText(mRecord->editorId);
    mNameEdit->setText(mRecord->name);
    mCostLimitSpin->setValue(mRecord->costLimit);
    mChargesSpin->setValue(mRecord->charges);
    mEnchantmentDataSpin->setValue(mRecord->enchantmentData);
    mChargeSpin->setValue(mRecord->charge);
    mDurationSpin->setValue(mRecord->duration);
    mMagnitudeSpin->setValue(mRecord->magnitude);
    mTypeCombo->setCurrentIndex(static_cast<int>(mRecord->type));
    mSoulGemCombo->setCurrentIndex(static_cast<int>(mRecord->soulGem));

    mPreviewTypeLabel->setText(QString("Enchantment: %1").arg(mRecord->editorId));
    mPreviewInfoLabel->setText(QString("Type: %1 | Soul Gem: %2")
        .arg(mTypeCombo->currentText())
        .arg(mSoulGemCombo->currentText()));
}

bool EnchEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getEnchCollection().searchId(editorId) >= 0)
    {
        if (editorId != mRecord->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "An enchantment with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void EnchEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    {
        auto results = ColumnValidator::validateEnch(*mRecord, mData, mOriginalEditorId);
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
    mRecord->name = mNameEdit->text();
    mRecord->costLimit = mCostLimitSpin->value();
    mRecord->charges = mChargesSpin->value();
    mRecord->enchantmentData = mEnchantmentDataSpin->value();
    mRecord->charge = static_cast<float>(mChargeSpin->value());
    mRecord->duration = mDurationSpin->value();
    mRecord->magnitude = static_cast<float>(mMagnitudeSpin->value());
    mRecord->type = static_cast<quint32>(mTypeCombo->currentIndex());
    mRecord->soulGem = static_cast<quint32>(mSoulGemCombo->currentIndex());

    accept();
}

void EnchEditor::previewEnchantment()
{
    QString enchType = mTypeCombo->currentText();
    QString info = QString("Type: %1 | Soul Gem: %2")
        .arg(enchType)
        .arg(mSoulGemCombo->currentText());
    mPreviewTypeLabel->setText(QString("Enchantment: %1").arg(mRecord->editorId));
    mPreviewInfoLabel->setText(info);

    // Try to find the associated MagicEffect's model path for 3D preview
    QString artPath;
    const auto& magicCollection = mData->getMagicCollection();
    for (int i = 0; i < magicCollection.size(); ++i) {
        const auto& effect = magicCollection.getRecord(i).get();
        if (!effect.modelPath.isEmpty()) {
            artPath = effect.modelPath;
            break;
        }
    }

    if (!artPath.isEmpty() && mPreviewViewport) {
        mPreviewViewport->clear();
        mPreviewViewport->loadNif(artPath);
    } else if (mPreviewViewport) {
        mPreviewViewport->clear();
    }

    showEffectPreview(artPath);
}

void EnchEditor::showEffectPreview(const QString& artPath)
{
    QDialog dlg(this);
    dlg.setWindowTitle("Enchantment Preview");
    dlg.setMinimumSize(600, 500);
    auto* layout = new QVBoxLayout(&dlg);

    if (artPath.isEmpty()) {
        auto* infoGroup = new QGroupBox("Enchantment Details");
        auto* infoLayout = new QVBoxLayout(infoGroup);
        
        auto* nameLabel = new QLabel(QString("<b>Enchantment:</b> %1").arg(mRecord->editorId));
        auto* typeLabel = new QLabel(QString("<b>Type:</b> %1").arg(mTypeCombo->currentText()));
        auto* soulLabel = new QLabel(QString("<b>Soul Gem:</b> %1").arg(mSoulGemCombo->currentText()));
        auto* costLabel = new QLabel(QString("<b>Cost Limit:</b> %1").arg(mRecord->costLimit));
        auto* chargesLabel = new QLabel(QString("<b>Charges:</b> %1").arg(mRecord->charges));
        
        infoLayout->addWidget(nameLabel);
        infoLayout->addWidget(typeLabel);
        infoLayout->addWidget(soulLabel);
        infoLayout->addWidget(costLabel);
        infoLayout->addWidget(chargesLabel);
        
        layout->addWidget(infoGroup);
        
        auto* noteLabel = new QLabel("Note: Enchantments do not have 3D models. Use the spell editor to preview visual effects.");
        noteLabel->setAlignment(Qt::AlignCenter);
        noteLabel->setStyleSheet("font-size: 12px; color: gray; font-style: italic;");
        noteLabel->setWordWrap(true);
        layout->addWidget(noteLabel);
    } else {
        auto* viewport = new NifViewportWidget(&dlg);
        layout->addWidget(viewport);
        viewport->loadNif(artPath);
    }

    auto* closeBtn = new QPushButton("Close");
    layout->addWidget(closeBtn);
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    dlg.exec();
}
