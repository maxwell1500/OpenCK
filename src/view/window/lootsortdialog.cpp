#include "lootsortdialog.hpp"
#include "../../model/world/data.hpp"
#include "../../model/tools/lootwrapper.hpp"
#include "logger.hpp"

#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QDateTime>

LootSortDialog::LootSortDialog(Data* data, QWidget* parent)
    : QDialog(parent)
    , mData(data)
    , m_lootWrapper(nullptr)
    , currentOrderList(nullptr)
    , sortedOrderList(nullptr)
    , logOutput(nullptr)
    , sortButton(nullptr)
    , applyButton(nullptr)
    , refreshButton(nullptr)
    , closeButton(nullptr)
    , statusLabel(nullptr)
    , lootStatusLabel(nullptr)
    , progressBar(nullptr)
    , m_currentPlugins()
    , m_sortedPlugins()
    , m_sortingComplete(false)
{
    LOG_DEBUG("LootSortDialog created");
    setWindowTitle("LOOT Sort Plugins");
    setMinimumSize(700, 500);

    m_lootWrapper = new LootWrapper(this);

    setupUI();
    loadCurrentPlugins();
    updateStatus();
}

LootSortDialog::~LootSortDialog()
{
    LOG_DEBUG("LootSortDialog destroyed");
}

void LootSortDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* statusGroup = new QGroupBox("LOOT Status", this);
    auto* statusLayout = new QHBoxLayout(statusGroup);

    lootStatusLabel = new QLabel("Checking LOOT installation...", this);
    statusLayout->addWidget(lootStatusLabel);

    refreshButton = new QPushButton("Refresh", this);
    statusLayout->addWidget(refreshButton);
    statusGroup->setLayout(statusLayout);
    mainLayout->addWidget(statusGroup);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    auto* currentGroup = new QGroupBox("Current Load Order", this);
    auto* currentLayout = new QVBoxLayout(currentGroup);
    currentOrderList = new QListWidget(this);
    currentOrderList->setSelectionMode(QAbstractItemView::SingleSelection);
    currentLayout->addWidget(currentOrderList);
    currentGroup->setLayout(currentLayout);
    splitter->addWidget(currentGroup);

    auto* sortedGroup = new QGroupBox("LOOT Sorted Order", this);
    auto* sortedLayout = new QVBoxLayout(sortedGroup);
    sortedOrderList = new QListWidget(this);
    sortedOrderList->setSelectionMode(QAbstractItemView::SingleSelection);
    sortedLayout->addWidget(sortedOrderList);
    sortedGroup->setLayout(sortedLayout);
    splitter->addWidget(sortedGroup);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter);

    auto* buttonLayout = new QHBoxLayout();
    sortButton = new QPushButton("Sort with LOOT", this);
    applyButton = new QPushButton("Apply Sorted Order", this);
    closeButton = new QPushButton("Close", this);
    buttonLayout->addWidget(sortButton);
    buttonLayout->addWidget(applyButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 0);
    progressBar->setVisible(false);
    mainLayout->addWidget(progressBar);

    statusLabel = new QLabel("Ready", this);
    mainLayout->addWidget(statusLabel);

    logOutput = new QTextBrowser(this);
    logOutput->setPlaceholderText("LOOT sorting log will appear here...");
    logOutput->setMaximumHeight(150);
    mainLayout->addWidget(logOutput);

    connect(sortButton, &QPushButton::clicked, this, &LootSortDialog::onSortWithLoot);
    connect(applyButton, &QPushButton::clicked, this, &LootSortDialog::onApplySortedOrder);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(refreshButton, &QPushButton::clicked, this, &LootSortDialog::onRefreshStatus);
}

void LootSortDialog::loadCurrentPlugins()
{
    m_currentPlugins.clear();
    currentOrderList->clear();
    sortedOrderList->clear();
    m_sortedPlugins.clear();
    m_sortingComplete = false;

    if (mData)
    {
        QStringList files = mData->getContentFiles();
        for (const QString& file : files)
        {
            m_currentPlugins.append(file);
            currentOrderList->addItem(file);
        }
    }

    logMessage(QString("Loaded %1 plugins").arg(m_currentPlugins.size()));
}

void LootSortDialog::updateStatus()
{
    if (m_lootWrapper->isAvailable())
    {
        lootStatusLabel->setText(QString("LOOT is installed: %1").arg(m_lootWrapper->getLootPath()));
        lootStatusLabel->setStyleSheet("color: green; font-weight: bold;");
        sortButton->setEnabled(true);
    }
    else
    {
        lootStatusLabel->setText("LOOT is not installed. Please install LOOT from https://loot.github.io/");
        lootStatusLabel->setStyleSheet("color: red; font-weight: bold;");
        sortButton->setEnabled(false);
    }
}

void LootSortDialog::logMessage(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    logOutput->append(QString("[%1] %2").arg(timestamp, message));
}

void LootSortDialog::onRefreshStatus()
{
    m_lootWrapper->setLootPath(QString());
    updateStatus();
    logMessage("LOOT status refreshed");
}

void LootSortDialog::onSortWithLoot()
{
    if (m_currentPlugins.isEmpty())
    {
        QMessageBox::warning(this, "No Plugins", "No plugins loaded to sort.");
        return;
    }

    if (!m_lootWrapper->isAvailable())
    {
        QMessageBox::warning(this, "LOOT Not Available",
            "LOOT is not installed or not found.\n\n"
            "Please install LOOT from https://loot.github.io/\n"
            "or set the LOOT path manually.");
        return;
    }

    statusLabel->setText("Sorting plugins with LOOT...");
    progressBar->setVisible(true);
    sortButton->setEnabled(false);
    applyButton->setEnabled(false);

    logMessage("Starting LOOT sort...");

    QCoreApplication::processEvents();

    m_sortedPlugins.clear();
    sortedOrderList->clear();

    QVector<QString> pluginsVector = m_currentPlugins.toVector();
    QVector<QString> sortedVector = m_lootWrapper->sortPlugins(pluginsVector);
    m_sortedPlugins = sortedVector.toList();

    for (const QString& plugin : m_sortedPlugins)
    {
        sortedOrderList->addItem(plugin);
    }

    m_sortingComplete = true;
    progressBar->setVisible(false);
    sortButton->setEnabled(true);
    applyButton->setEnabled(true);

    if (m_sortedPlugins == m_currentPlugins)
    {
        statusLabel->setText("Sorting complete - no changes needed");
        logMessage("LOOT sorting complete - load order unchanged");
    }
    else
    {
        statusLabel->setText("Sorting complete - review changes and click Apply");
        logMessage(QString("LOOT sorting complete - %1 plugins reordered").arg(m_sortedPlugins.size()));
    }
}

void LootSortDialog::onApplySortedOrder()
{
    if (!m_sortingComplete || m_sortedPlugins.isEmpty())
    {
        QMessageBox::warning(this, "No Sorted Order",
            "Please sort plugins with LOOT first.");
        return;
    }

    if (m_sortedPlugins == m_currentPlugins)
    {
        QMessageBox::information(this, "No Changes",
            "The sorted order is the same as the current order.");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Apply Sorted Order",
        "Apply the LOOT-sorted load order?\n\n"
        "This will update the plugin load order for the current session.",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        logMessage("Applying sorted order...");
        accept();
    }
}

QStringList LootSortDialog::getSortedPlugins() const
{
    if (m_sortingComplete && !m_sortedPlugins.isEmpty())
    {
        return m_sortedPlugins;
    }
    return m_currentPlugins;
}
