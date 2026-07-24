#include "worldspace_editor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "WorldspaceRecord.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QSpinBox>
#include <QMessageBox>

WorldspaceEditor::WorldspaceEditor(Data* data, WorldspaceRecord* worldspace, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mWorldspace(worldspace),
      mEditorIdEdit(nullptr),
      mNameEdit(nullptr),
      mIconPathEdit(nullptr),
      mWaterTypeSpin(nullptr),
      mTemplSpin(nullptr),
      mTerrainSpin(nullptr),
      mMapImageEdit(nullptr),
      mLodNoiseEdit(nullptr),
      mBillboardTextureEdit(nullptr),
      mMusicSpin(nullptr)
{
    setupUI();
    loadFromWorldspace();
}

void WorldspaceEditor::setupUI()
{
    setWindowTitle("Worldspace Editor");
    setMinimumSize(500, 500);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Worldspace Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mNameEdit = new QLineEdit();
    infoLayout->addRow("Name:", mNameEdit);

    mIconPathEdit = new QLineEdit();
    infoLayout->addRow("Icon Path:", mIconPathEdit);

    infoLayout->addRow("", new QLabel("<b>Properties</b>"));

    mWaterTypeSpin = new QSpinBox();
    mWaterTypeSpin->setRange(0, 999999);
    infoLayout->addRow("Water Type:", mWaterTypeSpin);

    mTemplSpin = new QSpinBox();
    mTemplSpin->setRange(0, 999999);
    infoLayout->addRow("Template:", mTemplSpin);

    mTerrainSpin = new QSpinBox();
    mTerrainSpin->setRange(0, 999999);
    infoLayout->addRow("Terrain:", mTerrainSpin);

    mMapImageEdit = new QLineEdit();
    infoLayout->addRow("Map Image:", mMapImageEdit);

    mLodNoiseEdit = new QLineEdit();
    infoLayout->addRow("LOD Noise:", mLodNoiseEdit);

    mBillboardTextureEdit = new QLineEdit();
    infoLayout->addRow("Billboard Texture:", mBillboardTextureEdit);

    mMusicSpin = new QSpinBox();
    mMusicSpin->setRange(0, 999999);
    infoLayout->addRow("Music:", mMusicSpin);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &WorldspaceEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void WorldspaceEditor::loadFromWorldspace()
{
    mEditorIdEdit->setText(mWorldspace->editorId);
    mNameEdit->setText(mWorldspace->name);
    mIconPathEdit->setText(mWorldspace->iconPath);
    mWaterTypeSpin->setValue(mWorldspace->waterType);
    mTemplSpin->setValue(mWorldspace->templ);
    mTerrainSpin->setValue(mWorldspace->terrain);
    mMapImageEdit->setText(mWorldspace->mapImage);
    mLodNoiseEdit->setText(mWorldspace->lodNoise);
    mBillboardTextureEdit->setText(mWorldspace->billboardTexture);
    mMusicSpin->setValue(mWorldspace->music);
}

bool WorldspaceEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getWorldspaceCollection().searchId(editorId) >= 0)
    {
        if (editorId != mWorldspace->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "A worldspace with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void WorldspaceEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    {
        auto results = ColumnValidator::validateWorldspace(*mWorldspace, mData);
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

    mWorldspace->editorId = mEditorIdEdit->text();
    mWorldspace->name = mNameEdit->text();
    mWorldspace->iconPath = mIconPathEdit->text();
    mWorldspace->waterType = mWaterTypeSpin->value();
    mWorldspace->templ = mTemplSpin->value();
    mWorldspace->terrain = mTerrainSpin->value();
    mWorldspace->mapImage = mMapImageEdit->text();
    mWorldspace->lodNoise = mLodNoiseEdit->text();
    mWorldspace->billboardTexture = mBillboardTextureEdit->text();
    mWorldspace->music = mMusicSpin->value();

    accept();
}
