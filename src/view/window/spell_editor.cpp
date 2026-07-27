#include "spell_editor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "Spellrecord.hpp"
#include "Magicrecord.hpp"
#include "nifviewportwidget.hpp"
#include "fieldvalidators.hpp"
#include "logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>
#include <QTabWidget>
#include <QDialog>
#include <QRegularExpression>

SpellEditor::SpellEditor(Data* data, SpellRecord* spell, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mRecord(spell),
      mEditorIdEdit(nullptr),
      mFullNameEdit(nullptr),
      mCostSpin(nullptr),
      mCastingSoundSpin(nullptr),
      mEffectsList(nullptr),
      mEffectCombo(nullptr),
      mMagnitudeSpin(nullptr),
      mDurationSpin(nullptr),
      mAreaSpin(nullptr),
      mPreviewNameLabel(nullptr),
      mPreviewSchoolLabel(nullptr),
      mPreviewStatsLabel(nullptr),
      mPreviewBtn(nullptr)
{
    mOriginalEditorId = mRecord->editorId;
    setupUI();
    loadFromSpell();
}

void SpellEditor::setupUI()
{
    setWindowTitle("Spell Editor");
    setMinimumSize(500, 500);

    auto* mainLayout = new QVBoxLayout(this);
    auto* tabs = new QTabWidget();

    // General Tab
    auto* infoGroup = new QGroupBox("Spell Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mFullNameEdit = new QLineEdit();
    infoLayout->addRow("Full Name:", mFullNameEdit);

    infoLayout->addRow("", new QLabel("<b>Properties</b>"));

    mCostSpin = new QSpinBox();
    setCostValidator(mCostSpin);
    infoLayout->addRow("Cast Point Cost:", mCostSpin);

    mCastingSoundSpin = new QSpinBox();
    setIntNonNegativeValidator(mCastingSoundSpin);
    infoLayout->addRow("Casting Sound ID:", mCastingSoundSpin);

    tabs->addTab(infoGroup, "General");

    // Effects Tab
    auto* effectsGroup = new QGroupBox("Magic Effects");
    auto* effectsLayout = new QVBoxLayout(effectsGroup);

    auto* addLayout = new QFormLayout();

    mEffectCombo = new QComboBox();
    const auto& magicCollection = mData->getMagicCollection();
    for (int i = 0; i < magicCollection.size(); ++i) {
        const auto& effect = magicCollection.getRecord(i).get();
        mEffectCombo->addItem(effect.editorId, effect.formId);
    }
    addLayout->addRow("Effect:", mEffectCombo);

    mMagnitudeSpin = new QDoubleSpinBox();
    setMagnitudeValidator(mMagnitudeSpin);
    mMagnitudeSpin->setValue(10.0);
    addLayout->addRow("Magnitude:", mMagnitudeSpin);

    mDurationSpin = new QSpinBox();
    setDurationValidator(mDurationSpin);
    mDurationSpin->setValue(10);
    addLayout->addRow("Duration (s):", mDurationSpin);

    mAreaSpin = new QSpinBox();
    setAreaValidator(mAreaSpin);
    mAreaSpin->setValue(0);
    addLayout->addRow("Area:", mAreaSpin);

    effectsLayout->addLayout(addLayout);

    auto* effectBtnLayout = new QHBoxLayout();
    auto* addEffectBtn = new QPushButton("Add Effect");
    auto* removeEffectBtn = new QPushButton("Remove Selected");
    effectBtnLayout->addWidget(addEffectBtn);
    effectBtnLayout->addWidget(removeEffectBtn);
    effectBtnLayout->addStretch();
    effectsLayout->addLayout(effectBtnLayout);

    connect(addEffectBtn, &QPushButton::clicked, this, &SpellEditor::addEffect);
    connect(removeEffectBtn, &QPushButton::clicked, this, &SpellEditor::removeEffect);

    mEffectsList = new QListWidget();
    effectsLayout->addWidget(mEffectsList, 1);

    connect(mEffectsList, &QListWidget::currentRowChanged, this, &SpellEditor::onEffectSelectionChanged);

    auto* previewGroup = new QGroupBox("Effect Preview");
    auto* previewLayout = new QVBoxLayout(previewGroup);

    mPreviewNameLabel = new QLabel("Select an effect to preview");
    mPreviewNameLabel->setStyleSheet("font-weight: bold;");
    previewLayout->addWidget(mPreviewNameLabel);

    mPreviewSchoolLabel = new QLabel("");
    previewLayout->addWidget(mPreviewSchoolLabel);

    mPreviewStatsLabel = new QLabel("");
    previewLayout->addWidget(mPreviewStatsLabel);

    mPreviewViewport = new NifViewportWidget(previewGroup);
    mPreviewViewport->setMinimumHeight(200);
    previewLayout->addWidget(mPreviewViewport, 1);

    mPreviewBtn = new QPushButton("Preview Spell (Full Size)");
    mPreviewBtn->setEnabled(false);
    previewLayout->addWidget(mPreviewBtn);

    connect(mPreviewBtn, &QPushButton::clicked, this, &SpellEditor::previewEffect);

    effectsLayout->addWidget(previewGroup);

    tabs->addTab(effectsGroup, "Effects");

    mainLayout->addWidget(tabs, 1);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &SpellEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void SpellEditor::loadFromSpell()
{
    mEditorIdEdit->setText(mRecord->editorId);
    mFullNameEdit->setText(mRecord->fullName);
    mCostSpin->setValue(mRecord->cost);
    mCastingSoundSpin->setValue(mRecord->castingSound);

    mEffectsList->clear();
    for (auto effId : mRecord->effects) {
        const auto& magicCollection = mData->getMagicCollection();
        QString label = QString::number(effId);
        for (int i = 0; i < magicCollection.size(); ++i) {
            if (magicCollection.getRecord(i).get().formId == effId) {
                label = magicCollection.getRecord(i).get().editorId;
                break;
            }
        }
        mEffectsList->addItem(label);
    }
}

void SpellEditor::addEffect()
{
    bool ok = false;
    quint32 formId = mEffectCombo->currentData().toUInt(&ok);
    if (!ok || formId == 0) return;

    QString desc = QString("%1 (Mag:%2 Dur:%3 Area:%4)")
        .arg(mEffectCombo->currentText())
        .arg(mMagnitudeSpin->value())
        .arg(mDurationSpin->value())
        .arg(mAreaSpin->value());

    mEffectsList->addItem(desc);
}

void SpellEditor::removeEffect()
{
    int row = mEffectsList->currentRow();
    if (row >= 0) {
        delete mEffectsList->takeItem(row);
        if (row < mRecord->effects.size()) {
            mRecord->effects.removeAt(row);
        }
    }
}

void SpellEditor::onEffectSelectionChanged()
{
    updateEffectPreview();
}

void SpellEditor::updateEffectPreview()
{
    int row = mEffectsList->currentRow();
    if (row < 0 || row >= mEffectsList->count()) {
        mPreviewNameLabel->setText("Select an effect to preview");
        mPreviewSchoolLabel->setText("");
        mPreviewStatsLabel->setText("");
        mPreviewBtn->setEnabled(false);
        return;
    }

    QString text = mEffectsList->item(row)->text();

    const auto& magicCollection = mData->getMagicCollection();
    quint32 effectFormId = 0;
    for (int i = 0; i < magicCollection.size(); ++i) {
        const auto& effect = magicCollection.getRecord(i).get();
        if (text.contains(effect.editorId)) {
            effectFormId = effect.formId;
            mPreviewNameLabel->setText(QString("Effect: %1").arg(effect.editorId));

            QString schoolName;
            switch (effect.schools) {
                case 0: schoolName = "Destruction"; break;
                case 1: schoolName = "Alteration"; break;
                case 2: schoolName = "Illusion"; break;
                case 3: schoolName = "Conjuration"; break;
                case 4: schoolName = "Mysticism"; break;
                case 5: schoolName = "Restoration"; break;
                default: schoolName = "Unknown"; break;
            }
            mPreviewSchoolLabel->setText(QString("School: %1").arg(schoolName));
            break;
        }
    }

    if (effectFormId == 0) {
        mPreviewNameLabel->setText(text);
        mPreviewSchoolLabel->setText("");
    }

    QRegularExpression re("Mag:(\\d+\\.?\\d*)\\s+Dur:(\\d+)\\s+Area:(\\d+)");
    QRegularExpressionMatch match = re.match(text);
    if (match.hasMatch()) {
        mPreviewStatsLabel->setText(QString("Magnitude: %1 | Duration: %2 | Area: %3")
            .arg(match.captured(1))
            .arg(match.captured(2))
            .arg(match.captured(3)));
    } else {
        mPreviewStatsLabel->setText("Magnitude: - | Duration: - | Area: -");
    }

    // Load 3D preview inline if model path exists
    QString artPath;
    for (int i = 0; i < magicCollection.size(); ++i) {
        const auto& effect = magicCollection.getRecord(i).get();
        if (text.contains(effect.editorId)) {
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

    mPreviewBtn->setEnabled(true);
}

void SpellEditor::previewEffect()
{
    int row = mEffectsList->currentRow();
    if (row < 0 || row >= mEffectsList->count()) return;

    QString text = mEffectsList->item(row)->text();

    const auto& magicCollection = mData->getMagicCollection();
    QString artPath;
    for (int i = 0; i < magicCollection.size(); ++i) {
        const auto& effect = magicCollection.getRecord(i).get();
        if (text.contains(effect.editorId)) {
            artPath = effect.modelPath;
            break;
        }
    }

    showEffectPreview(artPath);
}

void SpellEditor::showEffectPreview(const QString& artPath)
{
    QDialog dlg(this);
    dlg.setWindowTitle("Effect Preview");
    dlg.setMinimumSize(600, 500);
    auto* layout = new QVBoxLayout(&dlg);

    if (artPath.isEmpty()) {
        int row = mEffectsList->currentRow();
        if (row >= 0 && row < mEffectsList->count()) {
            QString text = mEffectsList->item(row)->text();
            
            auto* infoGroup = new QGroupBox("Effect Details");
            auto* infoLayout = new QVBoxLayout(infoGroup);
            
            const auto& magicCollection = mData->getMagicCollection();
            QString effectName = "Unknown Effect";
            QString schoolName = "Unknown";
            quint32 effectFormId = 0;
            
            for (int i = 0; i < magicCollection.size(); ++i) {
                const auto& effect = magicCollection.getRecord(i).get();
                if (text.contains(effect.editorId)) {
                    effectName = effect.editorId;
                    effectFormId = effect.formId;
                    switch (effect.schools) {
                        case 0: schoolName = "Destruction"; break;
                        case 1: schoolName = "Alteration"; break;
                        case 2: schoolName = "Illusion"; break;
                        case 3: schoolName = "Conjuration"; break;
                        case 4: schoolName = "Mysticism"; break;
                        case 5: schoolName = "Restoration"; break;
                        default: schoolName = "Unknown"; break;
                    }
                    break;
                }
            }
            
            auto* nameLabel = new QLabel(QString("<b>Effect:</b> %1").arg(effectName));
            auto* schoolLabel = new QLabel(QString("<b>School:</b> %1").arg(schoolName));
            
            QRegularExpression re("Mag:(\\d+\\.?\\d*)\\s+Dur:(\\d+)\\s+Area:(\\d+)");
            QRegularExpressionMatch match = re.match(text);
            
            if (match.hasMatch()) {
                auto* magLabel = new QLabel(QString("<b>Magnitude:</b> %1").arg(match.captured(1)));
                auto* durLabel = new QLabel(QString("<b>Duration:</b> %1s").arg(match.captured(2)));
                auto* areaLabel = new QLabel(QString("<b>Area:</b> %1").arg(match.captured(3)));
                infoLayout->addWidget(nameLabel);
                infoLayout->addWidget(schoolLabel);
                infoLayout->addWidget(magLabel);
                infoLayout->addWidget(durLabel);
                infoLayout->addWidget(areaLabel);
            } else {
                infoLayout->addWidget(nameLabel);
                infoLayout->addWidget(schoolLabel);
            }
            
            layout->addWidget(infoGroup);
            
            auto* noteLabel = new QLabel("Note: This effect does not have a 3D model associated with it.");
            noteLabel->setAlignment(Qt::AlignCenter);
            noteLabel->setStyleSheet("font-size: 12px; color: gray; font-style: italic;");
            noteLabel->setWordWrap(true);
            layout->addWidget(noteLabel);
        } else {
            auto* placeholder = new QLabel("No effect selected for preview.");
            placeholder->setAlignment(Qt::AlignCenter);
            placeholder->setStyleSheet("font-size: 14px; color: gray;");
            layout->addWidget(placeholder);
        }
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

bool SpellEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getSpellCollection().searchId(editorId) >= 0)
    {
        if (editorId != mRecord->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "A spell with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void SpellEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    {
        auto results = ColumnValidator::validateSpell(*mRecord, mData, mOriginalEditorId);
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
    mRecord->fullName = mFullNameEdit->text();
    mRecord->cost = mCostSpin->value();
    mRecord->castingSound = mCastingSoundSpin->value();

    // Sync effects - rebuild from list
    mRecord->effects.clear();
    const auto& magicCollection = mData->getMagicCollection();
    for (int i = 0; i < mEffectsList->count(); ++i) {
        QString text = mEffectsList->item(i)->text();
        for (int j = 0; j < magicCollection.size(); ++j) {
            const auto& effect = magicCollection.getRecord(j).get();
            if (text.contains(effect.editorId)) {
                mRecord->effects.append(effect.formId);
                break;
            }
        }
        if (mRecord->effects.size() <= i) {
            QString firstWord = text.section(' ', 0, 0);
            bool ok = false;
            quint32 val = firstWord.toUInt(&ok);
            if (ok && val != 0) {
                mRecord->effects.append(val);
            }
        }
    }

    accept();
}
