#include "spellwizard.hpp"

#include "../../../libs/files/esm/spellrecord.hpp"
#include "../../model/world/data.hpp"
#include "nifviewportwidget.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QDesktopServices>
#include <QUrl>

#include "logger.hpp"

SpellWizard::SpellWizard(Data* data, QWidget* parent)
    : QWizard(parent)
    , mData(data)
    , mBasicPage(nullptr)
    , mEffectsPage(nullptr)
    , mVisualsPage(nullptr)
    , mSummaryPage(nullptr)
{
    setWindowTitle(tr("Create New Spell"));
    setMinimumSize(550, 450);

    setupBasicPage();
    setupEffectsPage();
    setupVisualsPage();
    setupSummaryPage();

    populateEffectList();
}

void SpellWizard::setupBasicPage()
{
    mBasicPage = new QWizardPage();
    mBasicPage->setTitle(tr("Basic Spell Information"));
    mBasicPage->setSubTitle(tr("Choose the spell type, school, and basic properties."));

    auto* layout = new QFormLayout(mBasicPage);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setPlaceholderText(tr("e.g., MyCustomSpell"));
    layout->addRow(tr("Editor ID:"), mEditorIdEdit);

    mSpellNameEdit = new QLineEdit();
    mSpellNameEdit->setPlaceholderText(tr("e.g., Fire Storm"));
    layout->addRow(tr("Spell Name:"), mSpellNameEdit);

    mSpellTypeCombo = new QComboBox();
    mSpellTypeCombo->addItems({tr("Spell"), tr("Ability"), tr("Disease"),
                               tr("Power"), tr("Lesser Power")});
    layout->addRow(tr("Type:"), mSpellTypeCombo);

    mSchoolCombo = new QComboBox();
    mSchoolCombo->addItems({tr("Destruction"), tr("Restoration"), tr("Conjuration"),
                            tr("Alteration"), tr("Illusion"), tr("Mysticism")});
    layout->addRow(tr("School:"), mSchoolCombo);

    mCostSpin = new QSpinBox();
    mCostSpin->setRange(0, 999999);
    mCostSpin->setValue(50);
    layout->addRow(tr("Base Cost:"), mCostSpin);

    addPage(mBasicPage);
}

void SpellWizard::setupEffectsPage()
{
    mEffectsPage = new QWizardPage();
    mEffectsPage->setTitle(tr("Magic Effects"));
    mEffectsPage->setSubTitle(tr("Add one or more magic effects to this spell."));

    auto* layout = new QVBoxLayout(mEffectsPage);

    auto* addGroup = new QGroupBox(tr("Add Effect"));
    auto* addLayout = new QFormLayout(addGroup);

    mEffectCombo = new QComboBox();
    addLayout->addRow(tr("Effect:"), mEffectCombo);

    mMagnitudeSpin = new QDoubleSpinBox();
    mMagnitudeSpin->setRange(0.0, 9999.0);
    mMagnitudeSpin->setValue(10.0);
    mMagnitudeSpin->setDecimals(1);
    addLayout->addRow(tr("Magnitude:"), mMagnitudeSpin);

    mDurationSpin = new QSpinBox();
    mDurationSpin->setRange(0, 999999);
    mDurationSpin->setValue(10);
    addLayout->addRow(tr("Duration (s):"), mDurationSpin);

    mAreaSpin = new QSpinBox();
    mAreaSpin->setRange(0, 999999);
    mAreaSpin->setValue(0);
    addLayout->addRow(tr("Area:"), mAreaSpin);

    auto* addRemoveLayout = new QHBoxLayout();
    auto* addBtn = new QPushButton(tr("Add Effect"));
    auto* removeBtn = new QPushButton(tr("Remove Selected"));
    addRemoveLayout->addWidget(addBtn);
    addRemoveLayout->addWidget(removeBtn);
    addRemoveLayout->addStretch();
    addLayout->addRow(addRemoveLayout);

    connect(addBtn, &QPushButton::clicked, this, &SpellWizard::onAddEffect);
    connect(removeBtn, &QPushButton::clicked, this, &SpellWizard::onRemoveEffect);

    layout->addWidget(addGroup);

    mEffectList = new QListWidget();
    layout->addWidget(mEffectList, 1);

    addPage(mEffectsPage);
}

void SpellWizard::setupVisualsPage()
{
    mVisualsPage = new QWizardPage();
    mVisualsPage->setTitle(tr("Visuals & Audio"));
    mVisualsPage->setSubTitle(tr("Configure the spell's appearance and sound."));

    auto* layout = new QFormLayout(mVisualsPage);

    // Casting Art row with preview
    auto* castingArtRow = new QHBoxLayout();
    mCastingArtEdit = new QLineEdit();
    mCastingArtEdit->setPlaceholderText(tr("Casting art file path..."));
    mPreviewCastingArtBtn = new QPushButton(tr("Preview Art"));
    castingArtRow->addWidget(mCastingArtEdit, 1);
    castingArtRow->addWidget(mPreviewCastingArtBtn);
    layout->addRow(tr("Casting Art:"), castingArtRow);

    // Hit Effect row with preview
    auto* hitEffectRow = new QHBoxLayout();
    mHitEffectEdit = new QLineEdit();
    mHitEffectEdit->setPlaceholderText(tr("Hit effect file path..."));
    mPreviewHitEffectBtn = new QPushButton(tr("Preview Art"));
    hitEffectRow->addWidget(mHitEffectEdit, 1);
    hitEffectRow->addWidget(mPreviewHitEffectBtn);
    layout->addRow(tr("Hit Effect:"), hitEffectRow);

    mProjectileEdit = new QLineEdit();
    mProjectileEdit->setPlaceholderText(tr("Projectile file path..."));
    layout->addRow(tr("Projectile:"), mProjectileEdit);

    // Casting Sound row with preview
    auto* castingSoundRow = new QHBoxLayout();
    mCastingSoundSpin = new QSpinBox();
    mCastingSoundSpin->setRange(0, 999999);
    mPreviewCastingSoundBtn = new QPushButton(tr("Preview Sound"));
    castingSoundRow->addWidget(mCastingSoundSpin, 1);
    castingSoundRow->addWidget(mPreviewCastingSoundBtn);
    layout->addRow(tr("Casting Sound ID:"), castingSoundRow);

    addPage(mVisualsPage);

    connect(mPreviewCastingArtBtn, &QPushButton::clicked, this, &SpellWizard::onPreviewCastingArt);
    connect(mPreviewHitEffectBtn, &QPushButton::clicked, this, &SpellWizard::onPreviewHitEffect);
    connect(mPreviewCastingSoundBtn, &QPushButton::clicked, this, &SpellWizard::onPreviewCastingSound);
}

void SpellWizard::setupSummaryPage()
{
    mSummaryPage = new QWizardPage();
    mSummaryPage->setTitle(tr("Summary"));
    mSummaryPage->setSubTitle(tr("Review your spell before creating it."));

    auto* layout = new QVBoxLayout(mSummaryPage);

    mSummaryLabel = new QLabel();
    mSummaryLabel->setWordWrap(true);
    mSummaryLabel->setTextFormat(Qt::RichText);
    layout->addWidget(mSummaryLabel);

    addPage(mSummaryPage);

    connect(this, &QWizard::currentIdChanged, this, [this](int id) {
        if (page(id) == mSummaryPage) {
            updateSummary();
        }
    });
}

void SpellWizard::populateEffectList()
{
    if (!mData) return;

    const auto& magicCollection = mData->getMagicCollection();
    for (int i = 0; i < magicCollection.size(); ++i) {
        const auto& effect = magicCollection.getRecord(i).get();
        mEffectCombo->addItem(effect.editorId, effect.formId);
    }
}

void SpellWizard::onAddEffect()
{
    bool ok = false;
    quint32 formId = mEffectCombo->currentData().toUInt(&ok);
    if (!ok || formId == 0) return;

    QString desc = QString("%1 - Mag: %2, Dur: %3, Area: %4")
        .arg(mEffectCombo->currentText())
        .arg(mMagnitudeSpin->value())
        .arg(mDurationSpin->value())
        .arg(mAreaSpin->value());

    mEffectList->addItem(desc);
    mSelectedEffects.append(formId);
    mEffectMagnitudes.append(static_cast<float>(mMagnitudeSpin->value()));
    mEffectDurations.append(mDurationSpin->value());
    mEffectAreas.append(mAreaSpin->value());
}

void SpellWizard::onRemoveEffect()
{
    int row = mEffectList->currentRow();
    if (row < 0) return;

    delete mEffectList->takeItem(row);
    mSelectedEffects.removeAt(row);
    mEffectMagnitudes.removeAt(row);
    mEffectDurations.removeAt(row);
    mEffectAreas.removeAt(row);
}

void SpellWizard::updateSummary()
{
    QString html;
    html += QString("<h3>%1</h3>").arg(mSpellNameEdit->text().isEmpty()
                                            ? tr("(unnamed)") : mSpellNameEdit->text());

    html += "<p><b>Editor ID:</b> " + mEditorIdEdit->text() + "<br>";
    html += "<b>Type:</b> " + mSpellTypeCombo->currentText() + "<br>";
    html += "<b>School:</b> " + mSchoolCombo->currentText() + "<br>";
    html += "<b>Cost:</b> " + QString::number(mCostSpin->value()) + "</p>";

    html += "<h4>Effects</h4><ul>";
    for (int i = 0; i < mEffectList->count(); ++i) {
        html += "<li>" + mEffectList->item(i)->text() + "</li>";
    }
    if (mEffectList->count() == 0) {
        html += "<li><i>No effects added</i></li>";
    }
    html += "</ul>";

    html += "<h4>Visuals</h4><p>";
    html += "<b>Casting Art:</b> " + (mCastingArtEdit->text().isEmpty()
                                          ? tr("(none)") : mCastingArtEdit->text()) + "<br>";
    html += "<b>Hit Effect:</b> " + (mHitEffectEdit->text().isEmpty()
                                         ? tr("(none)") : mHitEffectEdit->text()) + "<br>";
    html += "<b>Projectile:</b> " + (mProjectileEdit->text().isEmpty()
                                         ? tr("(none)") : mProjectileEdit->text()) + "<br>";
    html += "<b>Casting Sound:</b> " + QString::number(mCastingSoundSpin->value()) + "</p>";

    mSummaryLabel->setText(html);
}

SpellRecord SpellWizard::result() const
{
    SpellRecord record;
    record.editorId = mEditorIdEdit->text();
    record.cost = mCostSpin->value();
    record.castingSound = mCastingSoundSpin->value();

    for (auto formId : mSelectedEffects) {
        record.effects.append(formId);
    }

    return record;
}

void SpellWizard::onPreviewCastingArt()
{
    QString path = mCastingArtEdit->text().trimmed();
    if (path.isEmpty())
    {
        QMessageBox::information(this, tr("Preview"), tr("No casting art path specified."));
        return;
    }
    QImage img = NifViewportWidget::loadTextureImage(path);
    if (img.isNull())
    {
        QMessageBox::warning(this, tr("Preview"), tr("Failed to load texture from:\n%1").arg(path));
        return;
    }
    QLabel* label = new QLabel();
    label->setPixmap(QPixmap::fromImage(img));
    label->setWindowTitle(tr("Casting Art Preview"));
    label->setAttribute(Qt::WA_DeleteOnClose);
    label->show();
}

void SpellWizard::onPreviewHitEffect()
{
    QString path = mHitEffectEdit->text().trimmed();
    if (path.isEmpty())
    {
        QMessageBox::information(this, tr("Preview"), tr("No hit effect path specified."));
        return;
    }
    QImage img = NifViewportWidget::loadTextureImage(path);
    if (img.isNull())
    {
        QMessageBox::warning(this, tr("Preview"), tr("Failed to load texture from:\n%1").arg(path));
        return;
    }
    QLabel* label = new QLabel();
    label->setPixmap(QPixmap::fromImage(img));
    label->setWindowTitle(tr("Hit Effect Preview"));
    label->setAttribute(Qt::WA_DeleteOnClose);
    label->show();
}

void SpellWizard::onPreviewCastingSound()
{
    int soundId = mCastingSoundSpin->value();
    if (soundId == 0)
    {
        QMessageBox::information(this, tr("Preview"), tr("No casting sound specified."));
        return;
    }
    QMessageBox::warning(this, tr("Preview"),
        tr("Sound preview is not available.\n"
           "Sound record (FormID %1) cannot be resolved to a file path.").arg(soundId));
}
