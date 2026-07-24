#include "materialeditor.hpp"
#include "../../model/world/data.hpp"
#include "logger.hpp"
#include "../../model/tools/columnvalidator.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>

MaterialEditor::MaterialEditor(Data* data, MaterialRecord* record, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mRecord(record)
{
    setWindowTitle("Material Editor");
    resize(600, 300);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    mTabWidget = new QTabWidget(this);
    
    // General tab
    QWidget* generalTab = new QWidget();
    QVBoxLayout* generalLayout = new QVBoxLayout(generalTab);

    // Editor ID
    QHBoxLayout* editorIdLayout = new QHBoxLayout();
    QLabel* editorIdLabel = new QLabel("Editor ID:");
    mEditorIdEdit = new QLineEdit("");
    mEditorIdEdit->setReadOnly(true);
    editorIdLayout->addWidget(editorIdLabel);
    editorIdLayout->addWidget(mEditorIdEdit);
    generalLayout->addLayout(editorIdLayout);

    // Form ID
    QHBoxLayout* formIdLayout = new QHBoxLayout();
    QLabel* formIdLabel = new QLabel("Form ID:");
    mFormIdEdit = new QLineEdit("0x00000000");
    mFormIdEdit->setReadOnly(true);
    formIdLayout->addWidget(formIdLabel);
    formIdLayout->addWidget(mFormIdEdit);
    generalLayout->addLayout(formIdLayout);

    // Material Name (BKMN)
    QHBoxLayout* materialNameLayout = new QHBoxLayout();
    QLabel* materialNameLbl = new QLabel("Material Name:");
    mMaterialNameEdit = new QLineEdit("");
    materialNameLayout->addWidget(materialNameLbl);
    materialNameLayout->addWidget(mMaterialNameEdit);
    generalLayout->addLayout(materialNameLayout);

    // BNAM
    QHBoxLayout* bnamLayout = new QHBoxLayout();
    QLabel* bnamLbl = new QLabel("BNAM:");
    mBnamEdit = new QLineEdit("");
    bnamLayout->addWidget(bnamLbl);
    bnamLayout->addWidget(mBnamEdit);
    generalLayout->addLayout(bnamLayout);

    // CNAM
    QHBoxLayout* cnamLayout = new QHBoxLayout();
    QLabel* cnamLbl = new QLabel("CNAM:");
    mCnamEdit = new QLineEdit("");
    cnamLayout->addWidget(cnamLbl);
    cnamLayout->addWidget(mCnamEdit);
    generalLayout->addLayout(cnamLayout);

    // Texture Path (MNAM)
    QHBoxLayout* textureLayout = new QHBoxLayout();
    QLabel* textureLbl = new QLabel("Texture Path:");
    mTexturePathEdit = new QLineEdit("");
    textureLayout->addWidget(textureLbl);
    textureLayout->addWidget(mTexturePathEdit);
    generalLayout->addLayout(textureLayout);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    mSaveButton = new QPushButton("Save");
    mCancelButton = new QPushButton("Cancel");
    buttonLayout->addWidget(mSaveButton);
    buttonLayout->addWidget(mCancelButton);
    generalLayout->addLayout(buttonLayout);

    mTabWidget->addTab(generalTab, "General");

    // Raw Sub Records tab
    QWidget* rawTab = new QWidget();
    QVBoxLayout* rawLayout = new QVBoxLayout(rawTab);
    QLabel* rawLabel = new QLabel("Raw Sub Records (empty for now):");
    rawLayout->addWidget(rawLabel);
    mTabWidget->addTab(rawTab, "Raw Data");

    mTabWidget->setCurrentIndex(0);

    mainLayout->addWidget(mTabWidget);

    connect(mSaveButton, &QPushButton::clicked, this, &MaterialEditor::saveChanges);
    connect(mCancelButton, &QPushButton::clicked, this, &MaterialEditor::reject);
}

MaterialEditor::~MaterialEditor()
{
}

void MaterialEditor::setupUI()
{
    // Already set up in constructor
}

void MaterialEditor::setRecord(MaterialRecord* record)
{
    mRecord = record;
    loadFromMaterial();
}

void MaterialEditor::loadFromMaterial()
{
    if (!mRecord) return;

    mEditorIdEdit->setText(mRecord->editorId);
    mFormIdEdit->setText(QString("0x%1").arg(mRecord->formId, 8, 16, QChar('0')).toUpper());
    mMaterialNameEdit->setText(mRecord->materialName);
    mBnamEdit->setText(mRecord->bnam);
    mCnamEdit->setText(mRecord->cnam);
    mTexturePathEdit->setText(mRecord->texturePath);
}

void MaterialEditor::saveToMaterial()
{
    if (!mRecord) return;

    mRecord->editorId = mEditorIdEdit->text();
    mRecord->materialName = mMaterialNameEdit->text();
    mRecord->bnam = mBnamEdit->text();
    mRecord->cnam = mCnamEdit->text();
    mRecord->texturePath = mTexturePathEdit->text();

    LOG_INFO(QString("Material '%1' updated").arg(mRecord->editorId));
}

bool MaterialEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    return true;
}

void MaterialEditor::saveChanges()
{
    if (!validate())
    {
        return;
    }

    auto results = ColumnValidator::validateMaterial(*mRecord, mData);
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

    saveToMaterial();
    accept();
}

void MaterialEditor::cancelEdit()
{
    reject();
}
