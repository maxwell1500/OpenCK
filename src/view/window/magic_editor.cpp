#include "magic_editor.hpp"

#include "../../model/world/data.hpp"
#include "Magicrecord.hpp"
#include "nifviewportwidget.hpp"
#include "../../model/tools/columnvalidator.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QPixmap>

MagicEditor::MagicEditor(Data* data, MagicRecord* magic, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mMagic(magic),
      mEditorIdEdit(nullptr),
      mSchoolsSpin(nullptr),
      mDamageTypeSpin(nullptr),
      mCastingSoundSpin(nullptr),
      mIconPathEdit(nullptr),
      mModelPathEdit(nullptr),
      mEffectsSpin(nullptr),
      mHitEffectArtPathEdit(nullptr),
      mIconBrowseBtn(nullptr),
      mHitEffectBrowseBtn(nullptr),
      mIconPreview(nullptr),
      mSecondaryEffectCombo(nullptr),
      mDualCastCostMultiplier(nullptr),
      mDualCastMagnitudeMultiplier(nullptr),
      mDualCastDurationMultiplier(nullptr)
{
    setupUI();
    loadFromMagic();
}

void MagicEditor::setupUI()
{
    setWindowTitle("Magic Effect Editor");
    setMinimumSize(450, 500);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Magic Effect Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mSchoolsSpin = new QSpinBox();
    mSchoolsSpin->setRange(0, 9999);
    infoLayout->addRow("Schools:", mSchoolsSpin);

    mDamageTypeSpin = new QSpinBox();
    mDamageTypeSpin->setRange(0, 9999);
    infoLayout->addRow("Damage Type:", mDamageTypeSpin);

    mCastingSoundSpin = new QSpinBox();
    mCastingSoundSpin->setRange(0, 9999);
    infoLayout->addRow("Casting Sound:", mCastingSoundSpin);

    mEffectsSpin = new QSpinBox();
    mEffectsSpin->setRange(0, 9999);
    infoLayout->addRow("Effects:", mEffectsSpin);

    mainLayout->addWidget(infoGroup);

    // Visual Assets group
    auto* visualGroup = new QGroupBox("Visual Assets");
    auto* visualLayout = new QFormLayout(visualGroup);

    // Icon Path row with browse and preview
    auto* iconRow = new QHBoxLayout();
    mIconPathEdit = new QLineEdit();
    mIconBrowseBtn = new QPushButton("Browse...");
    mIconPreview = new QLabel();
    mIconPreview->setFixedSize(32, 32);
    mIconPreview->setAlignment(Qt::AlignCenter);
    iconRow->addWidget(mIconPathEdit, 1);
    iconRow->addWidget(mIconBrowseBtn);
    iconRow->addWidget(mIconPreview);
    visualLayout->addRow("Icon Path:", iconRow);

    // Model Path
    mModelPathEdit = new QLineEdit();
    visualLayout->addRow("Model Path:", mModelPathEdit);

    // Hit Effect Art Path row with browse
    auto* hitEffectRow = new QHBoxLayout();
    mHitEffectArtPathEdit = new QLineEdit();
    mHitEffectBrowseBtn = new QPushButton("Browse...");
    hitEffectRow->addWidget(mHitEffectArtPathEdit, 1);
    hitEffectRow->addWidget(mHitEffectBrowseBtn);
    visualLayout->addRow("Hit Effect Art Path:", hitEffectRow);

    mainLayout->addWidget(visualGroup);

    // Dual Casting group
    auto* dualCastGroup = new QGroupBox("Dual Casting");
    auto* dualCastLayout = new QFormLayout(dualCastGroup);

    mSecondaryEffectCombo = new QComboBox();
    dualCastLayout->addRow("Secondary Effect:", mSecondaryEffectCombo);

    mDualCastCostMultiplier = new QDoubleSpinBox();
    mDualCastCostMultiplier->setRange(0.5, 3.0);
    mDualCastCostMultiplier->setSingleStep(0.1);
    mDualCastCostMultiplier->setDecimals(2);
    mDualCastCostMultiplier->setValue(1.5);
    dualCastLayout->addRow("Dual Cast Cost Multiplier:", mDualCastCostMultiplier);

    mDualCastMagnitudeMultiplier = new QDoubleSpinBox();
    mDualCastMagnitudeMultiplier->setRange(0.5, 3.0);
    mDualCastMagnitudeMultiplier->setSingleStep(0.1);
    mDualCastMagnitudeMultiplier->setDecimals(2);
    mDualCastMagnitudeMultiplier->setValue(2.2);
    dualCastLayout->addRow("Dual Cast Magnitude Multiplier:", mDualCastMagnitudeMultiplier);

    mDualCastDurationMultiplier = new QDoubleSpinBox();
    mDualCastDurationMultiplier->setRange(0.5, 3.0);
    mDualCastDurationMultiplier->setSingleStep(0.1);
    mDualCastDurationMultiplier->setDecimals(2);
    mDualCastDurationMultiplier->setValue(1.0);
    dualCastLayout->addRow("Dual Cast Duration Multiplier:", mDualCastDurationMultiplier);

    mainLayout->addWidget(dualCastGroup);

    // Populate the secondary effect combo from the magic collection
    if (mData)
    {
        const auto& magicCollection = mData->getMagicCollection();
        mSecondaryEffectCombo->addItem("(None)", 0);
        for (int i = 0; i < magicCollection.size(); ++i)
        {
            const auto& effect = magicCollection.getRecord(i).get();
            mSecondaryEffectCombo->addItem(effect.editorId, effect.formId);
        }
    }

    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &MagicEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(mIconBrowseBtn, &QPushButton::clicked, this, &MagicEditor::browseIconPath);
    connect(mHitEffectBrowseBtn, &QPushButton::clicked, this, &MagicEditor::browseHitEffectArt);
    connect(mIconPathEdit, &QLineEdit::textChanged, this, &MagicEditor::updateIconPreview);
}

void MagicEditor::browseIconPath()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Select Icon Texture",
        QString(), "Texture Files (*.dds *.tga *.png *.nif);;All Files (*.*)");
    if (filePath.isEmpty())
        return;
    mIconPathEdit->setText(filePath);
}

void MagicEditor::browseHitEffectArt()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Select Hit Effect Art",
        QString(), "Texture Files (*.dds *.tga *.png *.nif);;All Files (*.*)");
    if (filePath.isEmpty())
        return;
    mHitEffectArtPathEdit->setText(filePath);
}

void MagicEditor::updateIconPreview()
{
    QString path = mIconPathEdit->text().trimmed();
    if (path.isEmpty())
    {
        mIconPreview->clear();
        return;
    }
    QImage img = NifViewportWidget::loadTextureImage(path);
    if (img.isNull())
    {
        mIconPreview->setText("N/A");
        return;
    }
    QPixmap pm = QPixmap::fromImage(img.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    mIconPreview->setPixmap(pm);
}

void MagicEditor::loadFromMagic()
{
    mEditorIdEdit->setText(mMagic->editorId);
    mSchoolsSpin->setValue(static_cast<int>(mMagic->schools));
    mDamageTypeSpin->setValue(static_cast<int>(mMagic->damageType));
    mCastingSoundSpin->setValue(static_cast<int>(mMagic->castingSound));
    mIconPathEdit->setText(mMagic->iconPath);
    mModelPathEdit->setText(mMagic->modelPath);
    mEffectsSpin->setValue(static_cast<int>(mMagic->effects.size()));
}

void MagicEditor::saveToMagic()
{
    mMagic->editorId = mEditorIdEdit->text();
    mMagic->schools = static_cast<quint32>(mSchoolsSpin->value());
    mMagic->damageType = static_cast<quint32>(mDamageTypeSpin->value());
    mMagic->castingSound = static_cast<quint32>(mCastingSoundSpin->value());
    mMagic->iconPath = mIconPathEdit->text();
    mMagic->modelPath = mModelPathEdit->text();
    mMagic->effects.resize(mEffectsSpin->value());
}

bool MagicEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getMagicCollection().searchId(editorId) >= 0)
    {
        if (editorId != mMagic->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "A magic effect with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void MagicEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    auto results = ColumnValidator::validateMagic(*mMagic, mData);
    QStringList errorMessages;
    for (const auto& r : results)
    {
        if (r.severity == ColumnValidator::Severity::Error)
        {
            errorMessages << QString("%1: %2").arg(r.field, r.message);
        }
    }
    if (!errorMessages.isEmpty())
    {
        QMessageBox::warning(this, tr("Validation Errors"), errorMessages.join("\n"));
        return;
    }

    saveToMagic();
    accept();
}
