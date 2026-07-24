#include "bashedpatchdialog.hpp"

#include "../../model/tools/bashedpatchgenerator.hpp"
#include "../../model/world/data.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>

BashedPatchDialog::BashedPatchDialog(Data* data, QWidget* parent)
    : QDialog(parent)
    , mData(data)
    , mGenerator(nullptr)
{
    setupUI();
    setWindowTitle("Bashed Patch Generator");
    resize(500, 600);
}

BashedPatchDialog::~BashedPatchDialog()
{
    delete mGenerator;
}

void BashedPatchDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Patch Options group
    QGroupBox* optionsGroup = new QGroupBox("Patch Options", this);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    mNpcCheckBox = new QCheckBox("NPCs", optionsGroup);
    mNpcCheckBox->setChecked(true);
    optionsLayout->addWidget(mNpcCheckBox);

    mWeaponCheckBox = new QCheckBox("Weapons", optionsGroup);
    mWeaponCheckBox->setChecked(true);
    optionsLayout->addWidget(mWeaponCheckBox);

    mArmorCheckBox = new QCheckBox("Armor", optionsGroup);
    mArmorCheckBox->setChecked(true);
    optionsLayout->addWidget(mArmorCheckBox);

    mSpellCheckBox = new QCheckBox("Spells", optionsGroup);
    mSpellCheckBox->setChecked(true);
    optionsLayout->addWidget(mSpellCheckBox);

    mAlchemyCheckBox = new QCheckBox("Alchemy", optionsGroup);
    mAlchemyCheckBox->setChecked(true);
    optionsLayout->addWidget(mAlchemyCheckBox);

    mIngredientsCheckBox = new QCheckBox("Ingredients", optionsGroup);
    mIngredientsCheckBox->setChecked(true);
    optionsLayout->addWidget(mIngredientsCheckBox);

    mBooksCheckBox = new QCheckBox("Books", optionsGroup);
    mBooksCheckBox->setChecked(true);
    optionsLayout->addWidget(mBooksCheckBox);

    mEnchantmentsCheckBox = new QCheckBox("Enchantments", optionsGroup);
    mEnchantmentsCheckBox->setChecked(true);
    optionsLayout->addWidget(mEnchantmentsCheckBox);

    mContainersCheckBox = new QCheckBox("Containers", optionsGroup);
    mContainersCheckBox->setChecked(true);
    optionsLayout->addWidget(mContainersCheckBox);

    mMiscCheckBox = new QCheckBox("Misc Items", optionsGroup);
    mMiscCheckBox->setChecked(true);
    optionsLayout->addWidget(mMiscCheckBox);

    mActivatorsCheckBox = new QCheckBox("Activators", optionsGroup);
    mActivatorsCheckBox->setChecked(true);
    optionsLayout->addWidget(mActivatorsCheckBox);

    mRaceCheckBox = new QCheckBox("Race", optionsGroup);
    mRaceCheckBox->setChecked(true);
    optionsLayout->addWidget(mRaceCheckBox);

    mClassCheckBox = new QCheckBox("Class", optionsGroup);
    mClassCheckBox->setChecked(true);
    optionsLayout->addWidget(mClassCheckBox);

    mQuestCheckBox = new QCheckBox("Quest", optionsGroup);
    mQuestCheckBox->setChecked(true);
    optionsLayout->addWidget(mQuestCheckBox);

    mPackageCheckBox = new QCheckBox("Package", optionsGroup);
    mPackageCheckBox->setChecked(true);
    optionsLayout->addWidget(mPackageCheckBox);

    mFactCheckBox = new QCheckBox("Faction", optionsGroup);
    mFactCheckBox->setChecked(true);
    optionsLayout->addWidget(mFactCheckBox);

    mPerkCheckBox = new QCheckBox("Perk", optionsGroup);
    mPerkCheckBox->setChecked(true);
    optionsLayout->addWidget(mPerkCheckBox);

    // Select All / Deselect All buttons
    QHBoxLayout* selectLayout = new QHBoxLayout();
    mSelectAllButton = new QPushButton("Select All", optionsGroup);
    mDeselectAllButton = new QPushButton("Deselect All", optionsGroup);
    selectLayout->addWidget(mSelectAllButton);
    selectLayout->addWidget(mDeselectAllButton);
    optionsLayout->addLayout(selectLayout);

    connect(mSelectAllButton, &QPushButton::clicked, this, &BashedPatchDialog::onSelectAll);
    connect(mDeselectAllButton, &QPushButton::clicked, this, &BashedPatchDialog::onDeselectAll);

    mainLayout->addWidget(optionsGroup);

    // Output group
    QGroupBox* outputGroup = new QGroupBox("Output", this);
    QHBoxLayout* outputLayout = new QHBoxLayout(outputGroup);

    mOutputPathEdit = new QLineEdit(outputGroup);
    mOutputPathEdit->setPlaceholderText("Select output .esp file...");
    mOutputPathEdit->setText("BashedPatch.esp");
    outputLayout->addWidget(mOutputPathEdit);

    mBrowseButton = new QPushButton("Browse...", outputGroup);
    outputLayout->addWidget(mBrowseButton);

    connect(mBrowseButton, &QPushButton::clicked, this, &BashedPatchDialog::onBrowseOutput);

    mainLayout->addWidget(outputGroup);

    // Generate button
    mGenerateButton = new QPushButton("Generate Patch", this);
    mainLayout->addWidget(mGenerateButton);

    connect(mGenerateButton, &QPushButton::clicked, this, &BashedPatchDialog::onGeneratePatch);

    // Progress bar
    mProgressBar = new QProgressBar(this);
    mProgressBar->setRange(0, 100);
    mProgressBar->setValue(0);
    mainLayout->addWidget(mProgressBar);

    // Statistics label
    mStatsLabel = new QLabel("Plugins merged: 0 | Records merged: 0", this);
    mainLayout->addWidget(mStatsLabel);

    // Log text edit
    mLogTextEdit = new QTextEdit(this);
    mLogTextEdit->setReadOnly(true);
    mLogTextEdit->setPlaceholderText("Patch generation log will appear here...");
    mainLayout->addWidget(mLogTextEdit);
}

void BashedPatchDialog::onSelectAll()
{
    mNpcCheckBox->setChecked(true);
    mWeaponCheckBox->setChecked(true);
    mArmorCheckBox->setChecked(true);
    mSpellCheckBox->setChecked(true);
    mAlchemyCheckBox->setChecked(true);
    mIngredientsCheckBox->setChecked(true);
    mBooksCheckBox->setChecked(true);
    mEnchantmentsCheckBox->setChecked(true);
    mContainersCheckBox->setChecked(true);
    mMiscCheckBox->setChecked(true);
    mActivatorsCheckBox->setChecked(true);
    mRaceCheckBox->setChecked(true);
    mClassCheckBox->setChecked(true);
    mQuestCheckBox->setChecked(true);
    mPackageCheckBox->setChecked(true);
    mFactCheckBox->setChecked(true);
    mPerkCheckBox->setChecked(true);
}

void BashedPatchDialog::onDeselectAll()
{
    mNpcCheckBox->setChecked(false);
    mWeaponCheckBox->setChecked(false);
    mArmorCheckBox->setChecked(false);
    mSpellCheckBox->setChecked(false);
    mAlchemyCheckBox->setChecked(false);
    mIngredientsCheckBox->setChecked(false);
    mBooksCheckBox->setChecked(false);
    mEnchantmentsCheckBox->setChecked(false);
    mContainersCheckBox->setChecked(false);
    mMiscCheckBox->setChecked(false);
    mActivatorsCheckBox->setChecked(false);
    mRaceCheckBox->setChecked(false);
    mClassCheckBox->setChecked(false);
    mQuestCheckBox->setChecked(false);
    mPackageCheckBox->setChecked(false);
    mFactCheckBox->setChecked(false);
    mPerkCheckBox->setChecked(false);
}

void BashedPatchDialog::onBrowseOutput()
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Save Bashed Patch",
        mOutputPathEdit->text(),
        "ESM Files (*.esp);;All Files (*)"
    );

    if (!filePath.isEmpty())
    {
        mOutputPathEdit->setText(filePath);
    }
}

void BashedPatchDialog::onGeneratePatch()
{
    QString outputPath = mOutputPathEdit->text().trimmed();
    if (outputPath.isEmpty())
    {
        QMessageBox::warning(this, "Error", "Please specify an output file path.");
        return;
    }

    if (!mData)
    {
        QMessageBox::warning(this, "Error", "No data loaded. Please load plugins first.");
        return;
    }

    mGenerateButton->setEnabled(false);
    mLogTextEdit->clear();
    mProgressBar->setValue(0);
    mStatsLabel->setText("Generating patch...");
    QApplication::processEvents();

    BashedPatchGenerator::PatchConfig config;
    config.mergeNPCs = mNpcCheckBox->isChecked();
    config.mergeWeapons = mWeaponCheckBox->isChecked();
    config.mergeArmor = mArmorCheckBox->isChecked();
    config.mergeSpells = mSpellCheckBox->isChecked();
    config.mergeAlchemy = mAlchemyCheckBox->isChecked();
    config.mergeIngredients = mIngredientsCheckBox->isChecked();
    config.mergeBooks = mBooksCheckBox->isChecked();
    config.mergeEnchantments = mEnchantmentsCheckBox->isChecked();
    config.mergeContainers = mContainersCheckBox->isChecked();
    config.mergeMisc = mMiscCheckBox->isChecked();
    config.mergeActivators = mActivatorsCheckBox->isChecked();
    config.mergeRace = mRaceCheckBox->isChecked();
    config.mergeClass = mClassCheckBox->isChecked();
    config.mergeQuest = mQuestCheckBox->isChecked();
    config.mergePackage = mPackageCheckBox->isChecked();
    config.mergeFact = mFactCheckBox->isChecked();
    config.mergePerk = mPerkCheckBox->isChecked();

    mGenerator = new BashedPatchGenerator(mData);
    bool success = mGenerator->generatePatch(outputPath, config);

    mLogTextEdit->setText(mGenerator->getPatchLog());
    mProgressBar->setValue(100);

    QVector<QString> pluginList = mGenerator->getMergedPluginList();
    int recordCount = mGenerator->getMergedRecordCount();
    mStatsLabel->setText(QString("Plugins merged: %1 | Records merged: %2")
                         .arg(pluginList.size())
                         .arg(recordCount));

    if (success)
    {
        QMessageBox::information(this, "Success",
            "Bashed patch generated successfully!\n\n"
            "Output: " + outputPath + "\n"
            "Records merged: " + QString::number(recordCount));
    }
    else
    {
        QMessageBox::warning(this, "Error",
            "Failed to generate bashed patch.\n\n"
            "Check the log for details.");
    }

    mGenerateButton->setEnabled(true);
    delete mGenerator;
    mGenerator = nullptr;
}
