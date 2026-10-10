#include "materialeditor.hpp"
#include "materialpreviewwidget.hpp"
#include "../../model/world/data.hpp"
#include "logger.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "../../model/tools/materialruletemplate.hpp"
#include "../../model/tools/materialcompiler.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QDir>
#include <QFile>

MaterialEditor::MaterialEditor(Data* data, MaterialRecord* record, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mRecord(record)
{
    setWindowTitle("Material Editor");
    resize(760, 480);

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

    // Rule Templates tab (PBR texture slots + template rules + preview)
    setupRuleTemplateTab();

    loadTemplates();
    refreshSlotTable();
    updatePreview();

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

    loadFromMaterial();
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
    refreshSlotTable();
    updatePreview();
}

void MaterialEditor::saveToMaterial()
{
    if (!mRecord) return;

    mRecord->editorId = mEditorIdEdit->text();
    mRecord->materialName = mMaterialNameEdit->text();
    mRecord->bnam = mBnamEdit->text();
    mRecord->cnam = mCnamEdit->text();
    mRecord->texturePath = mTexturePathEdit->text();

    // Texture slot map from the rule templates tab.
    QMap<QString, QString> slotMap;
    for (int r = 0; r < mSlotTable->rowCount(); ++r)
    {
        QString name = mSlotTable->item(r, 0) ? mSlotTable->item(r, 0)->text() : QString();
        QString path = mSlotTable->item(r, 1) ? mSlotTable->item(r, 1)->text() : QString();
        if (!name.isEmpty())
            slotMap.insert(name, path);
    }
    mRecord->textureSlots = slotMap;

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

void MaterialEditor::setupRuleTemplateTab()
{
    QWidget* rtTab = new QWidget();
    QVBoxLayout* rtLayout = new QVBoxLayout(rtTab);

    // Template selection row
    QHBoxLayout* tmplRow = new QHBoxLayout();
    QLabel* tmplLabel = new QLabel("Rule Template:");
    mTemplateCombo = new QComboBox();
    mApplyTemplateButton = new QPushButton("Apply Template");
    tmplRow->addWidget(tmplLabel);
    tmplRow->addWidget(mTemplateCombo);
    tmplRow->addWidget(mApplyTemplateButton);
    rtLayout->addLayout(tmplRow);

    // Texture slot table
    mSlotTable = new QTableWidget(0, 2);
    mSlotTable->setHorizontalHeaderLabels({ "Slot", "Texture" });
    mSlotTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    mSlotTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    rtLayout->addWidget(mSlotTable);

    // Slot row buttons
    QHBoxLayout* rowButtons = new QHBoxLayout();
    mAddSlotButton = new QPushButton("Add Slot");
    mRemoveSlotButton = new QPushButton("Remove Slot");
    mBrowseButton = new QPushButton("Browse Texture...");
    rowButtons->addWidget(mAddSlotButton);
    rowButtons->addWidget(mRemoveSlotButton);
    rowButtons->addWidget(mBrowseButton);
    rowButtons->addStretch(1);
    rtLayout->addLayout(rowButtons);

    // Compile / preview row
    QHBoxLayout* compileRow = new QHBoxLayout();
    mCompileButton = new QPushButton("Compile & Preview");
    mStatusLabel = new QLabel("No material compiled yet.");
    mStatusLabel->setWordWrap(true);
    compileRow->addWidget(mCompileButton);
    compileRow->addWidget(mStatusLabel, 1);
    rtLayout->addLayout(compileRow);

    // Live PBR preview
    mPreview = new MaterialPreviewWidget();
    mPreview->setMinimumHeight(180);
    rtLayout->addWidget(mPreview);

    connect(mApplyTemplateButton, &QPushButton::clicked, this, &MaterialEditor::applyTemplate);
    connect(mCompileButton, &QPushButton::clicked, this, &MaterialEditor::compileAndPreview);
    connect(mBrowseButton, &QPushButton::clicked, this, &MaterialEditor::browseSlotTexture);
    connect(mAddSlotButton, &QPushButton::clicked, this, &MaterialEditor::addSlotRow);
    connect(mRemoveSlotButton, &QPushButton::clicked, this, &MaterialEditor::removeSlotRow);
    connect(mTemplateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { updatePreview(); });

    mTabWidget->addTab(rtTab, "Rule Templates");
}

void MaterialEditor::loadTemplates()
{
    mTemplates.clear();
    mTemplateCombo->blockSignals(true);
    mTemplateCombo->clear();

    // Real CK RuleTemplates live under <dataDir>/EditorFiles/RuleTemplates/ShaderModels.
    const QString rulesDir = mData->getPaths().dataDir.filePath(
        QStringLiteral("EditorFiles/RuleTemplates/ShaderModels"));
    MaterialRuleTemplate::loadDirectory(rulesDir, mTemplates);

    if (mTemplates.isEmpty())
    {
        // Fallback to built-in template names when the real files are absent.
        const QStringList names = MaterialRuleTemplate::builtinNames();
        for (const QString& name : names)
            mTemplates.append(MaterialRuleTemplate::builtinTemplate(name));
    }

    for (const MaterialRuleTemplate& t : mTemplates)
    {
        const QString label = t.displayName.isEmpty() ? t.name : t.displayName;
        mTemplateCombo->addItem(
            label + (t.category.isEmpty() ? QString() : QStringLiteral("  [%1]").arg(t.category)),
            t.name);
    }
    mTemplateCombo->blockSignals(false);
}

void MaterialEditor::refreshSlotTable()
{
    if (!mSlotTable)
        return;

    QMap<QString, QString> slotMap = mRecord ? mRecord->textureSlots : QMap<QString, QString>();
    mSlotTable->setRowCount(0);
    for (auto it = slotMap.constBegin(); it != slotMap.constEnd(); ++it)
    {
        const int row = mSlotTable->rowCount();
        mSlotTable->insertRow(row);
        mSlotTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        mSlotTable->setItem(row, 1, new QTableWidgetItem(it.value()));
    }
}

QMap<QString, QString> MaterialEditor::slotTableAsMap()
{
    QMap<QString, QString> slotMap;
    for (int r = 0; r < mSlotTable->rowCount(); ++r)
    {
        QString name = mSlotTable->item(r, 0) ? mSlotTable->item(r, 0)->text() : QString();
        QString path = mSlotTable->item(r, 1) ? mSlotTable->item(r, 1)->text() : QString();
        if (!name.isEmpty())
            slotMap.insert(name, path);
    }
    return slotMap;
}

void MaterialEditor::applyTemplate()
{
    const int idx = mTemplateCombo->currentIndex();
    if (idx < 0 || idx >= mTemplates.size())
    {
        mStatusLabel->setText("Select a rule template first.");
        return;
    }

    const MaterialRuleTemplate& tpl = mTemplates[idx];
    const QMap<QString, QString> current = slotTableAsMap();
    const MaterialRuleTemplate::ApplyMapResult applied = tpl.applyToSlotMap(current);

    mSlotTable->setRowCount(0);
    for (auto it = applied.slotPaths.constBegin(); it != applied.slotPaths.constEnd(); ++it)
    {
        const int row = mSlotTable->rowCount();
        mSlotTable->insertRow(row);
        mSlotTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        mSlotTable->setItem(row, 1, new QTableWidgetItem(it.value()));
    }

    mStatusLabel->setText(QStringLiteral("Applied template '%1': %2 texture slots, %3 operations.")
        .arg(tpl.name,
             QString::number(applied.slotPaths.size()),
             QString::number(applied.operations.size())));
    updatePreview();
}

void MaterialEditor::compileAndPreview()
{
    const int idx = mTemplateCombo->currentIndex();
    if (idx < 0 || idx >= mTemplates.size())
    {
        mStatusLabel->setText("Select a rule template first.");
        return;
    }

    const MaterialRuleTemplate& tpl = mTemplates[idx];
    const QMap<QString, QString> slotMap = slotTableAsMap();
    const QString textureRoot = mData->getPaths().dataDir.filePath(QStringLiteral("textures"));

    const MaterialCompileReport report = MaterialCompiler::compile(tpl, slotMap, textureRoot);
    mStatusLabel->setText(report.summary());

    // Show the first resolved texture in the live preview (fall back to Diffuse).
    QString texPath = report.resolvedSlots.value(QStringLiteral("Diffuse"));
    if (texPath.isEmpty() && !report.resolvedSlots.isEmpty())
        texPath = report.resolvedSlots.values().first();
    if (!texPath.isEmpty())
        mPreview->setTexture(texPath);
}

void MaterialEditor::browseSlotTexture()
{
    const int row = mSlotTable->currentRow();
    if (row < 0)
    {
        mStatusLabel->setText("Select a slot row to browse for a texture.");
        return;
    }

    const QString start = mData->getPaths().dataDir.filePath(QStringLiteral("textures"));
    const QString file = QFileDialog::getOpenFileName(
        this, "Select Texture", start,
        "Textures (*.dds *.tif *.tga *.png);;All files (*)");
    if (file.isEmpty())
        return;

    // Store the path relative to the texture root when possible.
    const QDir rootDir(mData->getPaths().dataDir.filePath(QStringLiteral("textures")));
    const QFileInfo fi(file);
    const QString rel = rootDir.relativeFilePath(file);
    const QString stored = (rel.startsWith(QLatin1Char('..')) ? fi.absoluteFilePath() : rel);

    if (!mSlotTable->item(row, 1))
        mSlotTable->setItem(row, 1, new QTableWidgetItem(stored));
    else
        mSlotTable->item(row, 1)->setText(stored);

    mPreview->setTexture(file);
}

void MaterialEditor::addSlotRow()
{
    const int row = mSlotTable->rowCount();
    mSlotTable->insertRow(row);
    mSlotTable->setItem(row, 0, new QTableWidgetItem("Diffuse"));
    mSlotTable->setItem(row, 1, new QTableWidgetItem(""));
    mSlotTable->setCurrentCell(row, 0);
}

void MaterialEditor::removeSlotRow()
{
    const int row = mSlotTable->currentRow();
    if (row >= 0)
        mSlotTable->removeRow(row);
}

void MaterialEditor::updatePreview()
{
    if (!mPreview)
        return;

    // Use the Diffuse (or first) slot texture as the preview albedo if present.
    const QMap<QString, QString> slotMap = slotTableAsMap();
    QString tex = slotMap.value(QStringLiteral("Diffuse"));
    if (tex.isEmpty() && !slotMap.isEmpty())
        tex = slotMap.values().first();
    if (!tex.isEmpty())
    {
        const QString abs = QDir(mData->getPaths().dataDir.filePath(QStringLiteral("textures")))
            .absoluteFilePath(tex);
        if (QFile::exists(abs))
            mPreview->setTexture(abs);
    }
}
