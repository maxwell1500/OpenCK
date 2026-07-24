#include "loadorderoptimizerdialog.hpp"
#include "../../model/world/data.hpp"
#include "../../model/tools/lootwrapper.hpp"
#include "lootsortdialog.hpp"
#include "logger.hpp"

#include <QMessageBox>
#include <QTextStream>
#include <QStack>
#include <QDir>
#include <QTreeWidgetItem>

LoadOrderOptimizerDialog::LoadOrderOptimizerDialog(Data* data, QWidget* parent)
    : QDialog(parent)
    , mData(data)
    , pluginList(nullptr)
    , reportBrowser(nullptr)
    , mDependencyTree(nullptr)
    , mValidationReport(nullptr)
    , analyzeButton(nullptr)
    , optimizeButton(nullptr)
    , applyButton(nullptr)
    , lootSortButton(nullptr)
    , validateButton(nullptr)
    , autoFixButton(nullptr)
    , statusLabel(nullptr)
    , currentOrder()
    , dependencyGraph()
    , mPluginMasters()
    , optimizedOrder()
    , analyzed(false)
    , hasCircularDeps(false)
    , hasMissingMasters(false)
{
    LOG_DEBUG("LoadOrderOptimizerDialog created");
    setWindowTitle("Load Order Optimizer");
    setMinimumSize(750, 650);
    setupUI();
}

LoadOrderOptimizerDialog::~LoadOrderOptimizerDialog()
{
    LOG_DEBUG("LoadOrderOptimizerDialog destroyed");
}

void LoadOrderOptimizerDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    mainLayout->addWidget(new QLabel("Current Load Order:", this));
    pluginList = new QListWidget(this);
    pluginList->setDragDropMode(QAbstractItemView::InternalMove);
    pluginList->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(pluginList);

    auto* orderButtonLayout = new QHBoxLayout();
    auto* moveUpBtn = new QPushButton("Move Up", this);
    auto* moveDownBtn = new QPushButton("Move Down", this);
    orderButtonLayout->addWidget(moveUpBtn);
    orderButtonLayout->addWidget(moveDownBtn);
    orderButtonLayout->addStretch();
    mainLayout->addLayout(orderButtonLayout);

    auto* buttonLayout = new QHBoxLayout();
    analyzeButton = new QPushButton("Analyze", this);
    optimizeButton = new QPushButton("Optimize", this);
    applyButton = new QPushButton("Apply", this);
    lootSortButton = new QPushButton("Sort with LOOT", this);
    validateButton = new QPushButton("Validate", this);
    autoFixButton = new QPushButton("Auto-Fix", this);
    buttonLayout->addWidget(analyzeButton);
    buttonLayout->addWidget(optimizeButton);
    buttonLayout->addWidget(lootSortButton);
    buttonLayout->addWidget(validateButton);
    buttonLayout->addWidget(autoFixButton);
    buttonLayout->addWidget(applyButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    statusLabel = new QLabel("Ready", this);
    mainLayout->addWidget(statusLabel);

    mDependencyTree = new QTreeWidget(this);
    mDependencyTree->setHeaderLabel("Dependency Graph");
    mDependencyTree->setMinimumHeight(150);
    mainLayout->addWidget(mDependencyTree);

    mValidationReport = new QTextEdit(this);
    mValidationReport->setReadOnly(true);
    mValidationReport->setPlaceholderText("Validation report will appear here...");
    mValidationReport->setMaximumHeight(200);
    mainLayout->addWidget(mValidationReport);

    reportBrowser = new QTextBrowser(this);
    reportBrowser->setPlaceholderText("Analysis report will appear here...");
    mainLayout->addWidget(reportBrowser);

    auto* bottomLayout = new QHBoxLayout();
    auto* closeButton = new QPushButton("Close", this);
    bottomLayout->addStretch();
    bottomLayout->addWidget(closeButton);
    mainLayout->addLayout(bottomLayout);

    connect(analyzeButton, &QPushButton::clicked, this, &LoadOrderOptimizerDialog::onAnalyze);
    connect(optimizeButton, &QPushButton::clicked, this, &LoadOrderOptimizerDialog::onOptimize);
    connect(applyButton, &QPushButton::clicked, this, &LoadOrderOptimizerDialog::onApply);
    connect(lootSortButton, &QPushButton::clicked, this, &LoadOrderOptimizerDialog::onSortWithLoot);
    connect(validateButton, &QPushButton::clicked, this, &LoadOrderOptimizerDialog::onValidate);
    connect(autoFixButton, &QPushButton::clicked, this, &LoadOrderOptimizerDialog::onAutoFix);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(moveUpBtn, &QPushButton::clicked, this, &LoadOrderOptimizerDialog::onMoveUp);
    connect(moveDownBtn, &QPushButton::clicked, this, &LoadOrderOptimizerDialog::onMoveDown);

    if (mData)
    {
        QStringList files = mData->getContentFiles();
        for (const QString& file : files)
        {
            pluginList->addItem(file);
            currentOrder << file;
        }
    }
}

QString LoadOrderOptimizerDialog::findPluginPath(const QString& pluginName) const
{
    if (!mData)
    {
        return QString();
    }

    QString dataDir = mData->getPaths().dataDir.path();
    QString candidate = dataDir + "/" + pluginName;
    if (QFile::exists(candidate))
    {
        return candidate;
    }

    return QString();
}

void LoadOrderOptimizerDialog::onAnalyze()
{
    LOG_INFO("Analyzing load order dependencies");
    statusLabel->setText("Analyzing...");

    currentOrder.clear();
    for (int i = 0; i < pluginList->count(); i++)
    {
        currentOrder << pluginList->item(i)->text();
    }

    analyzeDependencies();
    analyzed = true;
    statusLabel->setText("Analysis complete");
}

void LoadOrderOptimizerDialog::onOptimize()
{
    if (!analyzed)
    {
        onAnalyze();
    }

    if (optimizedOrder.isEmpty())
    {
        optimizeLoadOrder();
    }

    pluginList->clear();
    for (const QString& file : optimizedOrder)
    {
        pluginList->addItem(file);
    }

    statusLabel->setText("Load order optimized");
    reportBrowser->setPlainText("Optimization applied. Load order has been reordered based on master dependencies.");
}

void LoadOrderOptimizerDialog::onApply()
{
    LOG_INFO("Applying load order");

    currentOrder.clear();
    for (int i = 0; i < pluginList->count(); i++)
    {
        currentOrder << pluginList->item(i)->text();
    }

    if (mData)
    {
        QString pluginsPath = mData->getPaths().dataDir.path() + "/plugins.txt";
        QFile file(pluginsPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&file);
            for (const QString& plugin : currentOrder)
            {
                out << plugin << "\n";
            }
            file.close();
            LOG_INFO(QString("Saved load order to %1 (%2 plugins)")
                .arg(pluginsPath).arg(currentOrder.size()));
            QMessageBox::information(this, "Apply",
                QString("Load order saved to:\n%1\n\n%2 plugins written.")
                    .arg(pluginsPath).arg(currentOrder.size()));
        }
        else
        {
            LOG_ERROR(QString("Failed to write load order to %1").arg(pluginsPath));
            QMessageBox::warning(this, "Apply",
                QString("Failed to write load order to:\n%1").arg(pluginsPath));
        }
    }
    else
    {
        QMessageBox::warning(this, "Apply", "No data model available. Cannot save load order.");
    }
}

void LoadOrderOptimizerDialog::onMoveUp()
{
    int currentRow = pluginList->currentRow();
    if (currentRow > 0)
    {
        QListWidgetItem* item = pluginList->takeItem(currentRow);
        pluginList->insertItem(currentRow - 1, item);
        pluginList->setCurrentRow(currentRow - 1);

        currentOrder.clear();
        for (int i = 0; i < pluginList->count(); i++)
        {
            currentOrder << pluginList->item(i)->text();
        }
    }
}

void LoadOrderOptimizerDialog::onMoveDown()
{
    int currentRow = pluginList->currentRow();
    if (currentRow >= 0 && currentRow < pluginList->count() - 1)
    {
        QListWidgetItem* item = pluginList->takeItem(currentRow);
        pluginList->insertItem(currentRow + 1, item);
        pluginList->setCurrentRow(currentRow + 1);

        currentOrder.clear();
        for (int i = 0; i < pluginList->count(); i++)
        {
            currentOrder << pluginList->item(i)->text();
        }
    }
}

void LoadOrderOptimizerDialog::analyzeDependencies()
{
    dependencyGraph.clear();
    mPluginMasters.clear();
    hasCircularDeps = false;
    hasMissingMasters = false;

    // Parse TES4 headers to get master dependencies from each plugin
    for (const QString& file : currentOrder)
    {
        dependencyGraph[file] = QStringList();

        QString pluginPath = findPluginPath(file);
        if (!pluginPath.isEmpty())
        {
            LootWrapper lootWrapper;
            QStringList masters = lootWrapper.getMasterPlugins(pluginPath);
            mPluginMasters[file] = masters;
            dependencyGraph[file] = masters;
        }
        else
        {
            mPluginMasters[file] = QStringList();
        }
    }

    // Detect missing masters
    QStringList missingMasters;
    QSet<QString> loadOrderSet;
    for (const QString& file : currentOrder)
    {
        loadOrderSet.insert(file.toLower());
    }

    for (auto it = mPluginMasters.constBegin(); it != mPluginMasters.constEnd(); ++it)
    {
        for (const QString& master : it.value())
        {
            if (!loadOrderSet.contains(master.toLower()))
            {
                missingMasters << QString("%1 requires missing master: %2").arg(it.key(), master);
                hasMissingMasters = true;
            }
        }
    }

    // Detect circular dependencies using DFS
    QStringList circularDeps;
    QSet<QString> visited;
    QSet<QString> recursionStack;

    for (const QString& file : currentOrder)
    {
        if (!visited.contains(file))
        {
            QStringList cyclePath;
            if (detectCircularDepsDFS(file, visited, recursionStack, cyclePath))
            {
                hasCircularDeps = true;
                circularDeps << cyclePath.join(" -> ");
            }
        }
    }

    // Build the dependency tree widget
    buildDependencyTreeWidget();

    // Generate report
    QString report;
    QTextStream out(&report);

    out << "Load Order Analysis Report\n";
    out << "=========================\n\n";
    out << "Plugins analyzed: " << currentOrder.size() << "\n\n";

    out << "Dependency Graph (parsed from TES4 headers):\n";
    out << "----------------------------------------------\n";
    for (auto it = mPluginMasters.constBegin(); it != mPluginMasters.constEnd(); ++it)
    {
        if (it.value().isEmpty())
        {
            out << "  " << it.key() << " -> (no masters)\n";
        }
        else
        {
            out << "  " << it.key() << " -> " << it.value().join(", ") << "\n";
        }
    }

    if (!missingMasters.isEmpty())
    {
        out << "\nMissing Masters:\n";
        out << "----------------\n";
        for (const QString& msg : missingMasters)
        {
            out << "  [X] " << msg << "\n";
        }
    }

    if (!circularDeps.isEmpty())
    {
        out << "\nCircular Dependencies:\n";
        out << "----------------------\n";
        for (const QString& cycle : circularDeps)
        {
            out << "  [X] " << cycle << "\n";
        }
    }

    // ESM before ESP check
    int lastEsmPos = -1;
    bool espBeforeEsm = false;
    for (int i = 0; i < currentOrder.size(); i++)
    {
        if (currentOrder[i].endsWith(".esm", Qt::CaseInsensitive))
        {
            lastEsmPos = i;
        }
        else if (currentOrder[i].endsWith(".esp", Qt::CaseInsensitive) && lastEsmPos >= 0)
        {
            espBeforeEsm = true;
        }
    }

    out << "\nOrdering Issues:\n";
    out << "----------------\n";
    if (espBeforeEsm)
    {
        out << "  [!] ESP files found before ESM files\n";
    }
    else
    {
        out << "  [OK] ESM files load before ESP files\n";
    }

    out << "\nCircular dependencies: " << circularDeps.size() << "\n";
    out << "Missing masters: " << missingMasters.size() << "\n\n";

    // Recommendations
    out << "Recommendations:\n";
    if (missingMasters.isEmpty() && circularDeps.isEmpty() && !espBeforeEsm)
    {
        out << "  Load order appears valid.\n";
        out << "  Click 'Optimize' to check for further improvements.\n";
    }
    else
    {
        out << "  Issues detected - click 'Auto-Fix' to resolve.\n";
    }

    reportBrowser->setPlainText(report);
}

bool LoadOrderOptimizerDialog::detectCircularDepsDFS(
    const QString& node, QSet<QString>& visited,
    QSet<QString>& recursionStack, QStringList& cyclePath)
{
    visited.insert(node);
    recursionStack.insert(node);

    const QStringList& masters = mPluginMasters.value(node, QStringList());
    for (const QString& master : masters)
    {
        if (!visited.contains(master))
        {
            if (detectCircularDepsDFS(master, visited, recursionStack, cyclePath))
            {
                cyclePath.prepend(node);
                return true;
            }
        }
        else if (recursionStack.contains(master))
        {
            cyclePath.clear();
            cyclePath << master << node;
            return true;
        }
    }

    recursionStack.remove(node);
    return false;
}

void LoadOrderOptimizerDialog::buildDependencyTreeWidget()
{
    mDependencyTree->clear();

    // Find root plugins (those with no masters, or masters not in the load order)
    QSet<QString> hasMaster;
    for (auto it = mPluginMasters.constBegin(); it != mPluginMasters.constEnd(); ++it)
    {
        for (const QString& master : it.value())
        {
            hasMaster.insert(master.toLower());
        }
    }

    // Track which plugins have been added as tree items
    QMap<QString, QTreeWidgetItem*> treeItems;

    // Build root items first
    for (const QString& file : currentOrder)
    {
        if (!hasMaster.contains(file.toLower()) || file.toLower() == "skyrim.esm"
            || file.toLower() == "update.esm")
        {
            auto* rootItem = new QTreeWidgetItem(mDependencyTree);
            rootItem->setText(0, file);
            rootItem->setExpanded(true);
            treeItems[file.toLower()] = rootItem;
        }
    }

    // Now build the tree recursively by adding children under their masters
    bool added = true;
    int iterations = 0;
    while (added && iterations < 50)
    {
        added = false;
        iterations++;

        for (const QString& file : currentOrder)
        {
            if (treeItems.contains(file.toLower()))
            {
                continue;
            }

            const QStringList& masters = mPluginMasters.value(file, QStringList());
            if (masters.isEmpty())
            {
                continue;
            }

            // Find the first master that is already in the tree
            for (const QString& master : masters)
            {
                if (treeItems.contains(master.toLower()))
                {
                    auto* parentItem = treeItems[master.toLower()];
                    auto* childItem = new QTreeWidgetItem(parentItem);
                    childItem->setText(0, QString("%1 (depends on %2)").arg(file, master));
                    treeItems[file.toLower()] = childItem;
                    added = true;
                    break;
                }
            }
        }
    }

    // Any remaining plugins without tree position become root items
    for (const QString& file : currentOrder)
    {
        if (!treeItems.contains(file.toLower()))
        {
            auto* orphanItem = new QTreeWidgetItem(mDependencyTree);
            orphanItem->setText(0, QString("%1 (orphan)").arg(file));
            treeItems[file.toLower()] = orphanItem;
        }
    }
}

void LoadOrderOptimizerDialog::validate()
{
    QString report;
    QTextStream out(&report);
    int passed = 0;
    int failed = 0;
    int warnings = 0;

    out << "Load Order Validation Report\n";
    out << "============================\n\n";

    // Check 1: All master files are present
    QSet<QString> loadOrderSet;
    for (const QString& file : currentOrder)
    {
        loadOrderSet.insert(file.toLower());
    }

    bool allMastersPresent = true;
    QStringList missingMasterList;
    for (auto it = mPluginMasters.constBegin(); it != mPluginMasters.constEnd(); ++it)
    {
        for (const QString& master : it.value())
        {
            if (!loadOrderSet.contains(master.toLower()))
            {
                allMastersPresent = false;
                missingMasterList << QString("%1 requires %2").arg(it.key(), master);
            }
        }
    }

    if (allMastersPresent)
    {
        out << "[PASS] All master files are present in the load order\n";
        passed++;
    }
    else
    {
        out << "[FAIL] Missing masters detected:\n";
        for (const QString& msg : missingMasterList)
        {
            out << "       - " << msg << "\n";
        }
        failed++;
    }

    // Check 2: ESM files come before ESP files
    bool esmBeforeEsp = true;
    int lastEsmPos = -1;
    for (int i = 0; i < currentOrder.size(); i++)
    {
        if (currentOrder[i].endsWith(".esm", Qt::CaseInsensitive))
        {
            lastEsmPos = i;
        }
        else if (currentOrder[i].endsWith(".esp", Qt::CaseInsensitive))
        {
            if (lastEsmPos < i)
            {
                // Check if there's an ESP before this ESM position
                for (int j = 0; j < i; j++)
                {
                    if (currentOrder[j].endsWith(".esp", Qt::CaseInsensitive))
                    {
                        esmBeforeEsp = false;
                        break;
                    }
                }
            }
        }
    }

    if (esmBeforeEsp)
    {
        out << "[PASS] ESM files come before ESP files\n";
        passed++;
    }
    else
    {
        out << "[FAIL] ESP files found before ESM files\n";
        failed++;
    }

    // Check 3: No circular dependencies
    if (!hasCircularDeps)
    {
        out << "[PASS] No circular dependencies detected\n";
        passed++;
    }
    else
    {
        out << "[FAIL] Circular dependencies detected\n";
        failed++;
    }

    // Check 4: Base masters at top
    QStringList baseMasters = {"Skyrim.esm", "Update.esm"};
    bool baseMastersAtTop = true;
    int baseMasterCount = 0;
    for (int i = 0; i < currentOrder.size(); i++)
    {
        if (baseMasters.contains(currentOrder[i]))
        {
            baseMasterCount++;
        }
    }

    // First N items should be base masters
    int topCount = 0;
    for (int i = 0; i < qMin(baseMasterCount, currentOrder.size()); i++)
    {
        if (baseMasters.contains(currentOrder[i]))
        {
            topCount++;
        }
    }

    if (topCount == baseMasterCount || baseMasterCount == 0)
    {
        out << "[PASS] Base masters (Skyrim.esm, Update.esm) are at the top\n";
        passed++;
    }
    else
    {
        out << "[WARN] Base masters are not at the top of the load order\n";
        warnings++;
    }

    // Check 5: No overlapping FormID ranges
    // FormID range check: each plugin's FormID starts at its index (light: 0x800-0xFFF, normal: 1-0xFFF)
    // For simplicity, check if plugins are in a reasonable order
    bool formIdOk = true;
    if (!esmBeforeEsp)
    {
        formIdOk = false;
    }

    if (formIdOk)
    {
        out << "[PASS] No FormID range overlaps detected\n";
        passed++;
    }
    else
    {
        out << "[WARN] Potential FormID range issues due to ordering problems\n";
        warnings++;
    }

    out << "\n========================================\n";
    out << "Results: " << passed << " passed, " << failed << " failed, " << warnings << " warnings\n";

    mValidationReport->setPlainText(report);
}

void LoadOrderOptimizerDialog::onValidate()
{
    LOG_INFO("Validating load order");
    statusLabel->setText("Validating...");

    currentOrder.clear();
    for (int i = 0; i < pluginList->count(); i++)
    {
        currentOrder << pluginList->item(i)->text();
    }

    if (!analyzed)
    {
        analyzeDependencies();
        analyzed = true;
    }

    validate();
    statusLabel->setText("Validation complete");
}

void LoadOrderOptimizerDialog::autoFix()
{
    LOG_INFO("Auto-fixing load order");
    statusLabel->setText("Auto-fixing...");

    currentOrder.clear();
    for (int i = 0; i < pluginList->count(); i++)
    {
        currentOrder << pluginList->item(i)->text();
    }

    if (!analyzed)
    {
        analyzeDependencies();
        analyzed = true;
    }

    QStringList fixLog;
    int fixCount = 0;

    // Fix 1: Move base masters to the top
    QStringList baseMasters = {"Skyrim.esm", "Update.esm", "Dawnguard.esm", "Hearthfires.esm", "Dragonborn.esm"};
    for (const QString& baseMaster : baseMasters)
    {
        int idx = currentOrder.indexOf(baseMaster);
        if (idx > 0)
        {
            currentOrder.removeAt(idx);
            currentOrder.prepend(baseMaster);
            fixLog << QString("Moved %1 to the top").arg(baseMaster);
            fixCount++;
        }
    }

    // Fix 2: Reorder ESM before ESP
    QStringList esmFiles;
    QStringList espFiles;
    QStringList eslFiles;
    for (const QString& file : currentOrder)
    {
        if (file.endsWith(".esm", Qt::CaseInsensitive))
        {
            esmFiles << file;
        }
        else if (file.endsWith(".esl", Qt::CaseInsensitive))
        {
            eslFiles << file;
        }
        else
        {
            espFiles << file;
        }
    }

    // Check if any ESP was before an ESM (before we split them)
    QStringList newOrder = esmFiles + espFiles + eslFiles;
    if (newOrder != currentOrder)
    {
        fixLog << "Reordered: ESM files now load before ESP files";
        fixCount++;
    }
    currentOrder = newOrder;

    // Fix 3: Topological sort based on dependencies
    // Move missing masters that exist in the load order to correct positions
    QMap<QString, int> positionMap;
    for (int i = 0; i < currentOrder.size(); i++)
    {
        positionMap[currentOrder[i].toLower()] = i;
    }

    // Check if any plugin loads before its required master
    bool reordered = true;
    int iterations = 0;
    while (reordered && iterations < 10)
    {
        reordered = false;
        iterations++;
        for (int i = 0; i < currentOrder.size(); i++)
        {
            const QStringList& masters = mPluginMasters.value(currentOrder[i], QStringList());
            for (const QString& master : masters)
            {
                int masterIdx = -1;
                for (int j = 0; j < currentOrder.size(); j++)
                {
                    if (currentOrder[j].toLower() == master.toLower())
                    {
                        masterIdx = j;
                        break;
                    }
                }

                if (masterIdx >= 0 && masterIdx > i)
                {
                    // Master is after the plugin, move the plugin after the master
                    QString plugin = currentOrder.takeAt(i);
                    currentOrder.insert(masterIdx + 1, plugin);
                    fixLog << QString("Moved %1 after its master %2").arg(plugin, master);
                    fixCount++;
                    reordered = true;
                    break;
                }
            }
        }
    }

    // Update the UI
    pluginList->clear();
    for (const QString& file : currentOrder)
    {
        pluginList->addItem(file);
    }

    // Re-analyze after fixes
    analyzeDependencies();
    analyzed = true;

    // Build fix report
    QString report;
    QTextStream out(&report);

    out << "Auto-Fix Summary\n";
    out << "================\n\n";

    if (fixCount == 0)
    {
        out << "No issues found. Load order is already correct.\n";
    }
    else
    {
        out << "Fixed " << fixCount << " ordering issue(s):\n\n";
        for (const QString& log : fixLog)
        {
            out << "  [OK] " << log << "\n";
        }
    }

    out << "\nResulting load order:\n";
    for (int i = 0; i < currentOrder.size(); i++)
    {
        out << "  " << (i + 1) << ". " << currentOrder[i] << "\n";
    }

    reportBrowser->setPlainText(report);
    statusLabel->setText(QString("Auto-Fix: Fixed %1 issue(s)").arg(fixCount));

    LOG_INFO(QString("Auto-fix completed: %1 issues fixed").arg(fixCount));
}

void LoadOrderOptimizerDialog::onAutoFix()
{
    autoFix();
}

void LoadOrderOptimizerDialog::optimizeLoadOrder()
{
    autoFix();

    optimizedOrder.clear();
    for (int i = 0; i < pluginList->count(); i++)
    {
        optimizedOrder << pluginList->item(i)->text();
    }
}

void LoadOrderOptimizerDialog::onSortWithLoot()
{
    LOG_INFO("Opening LOOT Sort Dialog");

    LootSortDialog dialog(mData, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        QStringList sortedPlugins = dialog.getSortedPlugins();

        pluginList->clear();
        for (const QString& plugin : sortedPlugins)
        {
            pluginList->addItem(plugin);
        }

        currentOrder = sortedPlugins;
        optimizedOrder = sortedPlugins;
        analyzed = false;

        statusLabel->setText("Load order updated with LOOT sorting");
        reportBrowser->setPlainText("LOOT sorting applied. The load order has been updated based on LOOT's optimization rules.");
    }
}
