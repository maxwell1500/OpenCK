#include "datadialog.hpp"
#include "../../../ui/ui_datadialog.h"

#include "../messageboxhelper.hpp"
#include "logger.hpp"
#include "filepaths.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QMessageBox>
#include <QSettings>

DataDialog::DataDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::datadialog)
{
    ui->setupUi(this);
}

DataDialog::~DataDialog()
{
    delete ui;
}

void DataDialog::setUp(const QString& path)
{
    dataPath = path;
    currentGame = Game_None;
    pendingLoadErrors.clear();

    LOG_INFO(QString("DataDialog::setUp starting with path='%1'").arg(path));

    // Detect which game this path belongs to
    auto detected = FilePaths::detectGames();
    LOG_INFO(QString("DataDialog::setUp: detectGames returned %1 entries").arg(detected.size()));
    for (const auto& game : detected)
    {
        if (QDir(game.dataPath) == QDir(path))
        {
            currentGame = game.gameId;
            break;
        }
    }
    LOG_INFO(QString("DataDialog::setUp: currentGame=%1").arg(static_cast<int>(currentGame)));

    populateGameSelector();
    LOG_INFO("DataDialog::setUp: populateGameSelector done");
    configureTable();
    LOG_INFO("DataDialog::setUp: configureTable done");
    configureList();
    LOG_INFO("DataDialog::setUp: configureList done");

    if (dataTable)
        pendingLoadErrors = dataTable->getLoadErrors();
    LOG_INFO(QString("DataDialog::setUp complete, %1 load errors").arg(pendingLoadErrors.size()));
}

void DataDialog::populateGameSelector()
{
    QComboBox* combo = gameSelector();
    combo->clear();

    auto detected = FilePaths::detectGames();
    bool currentPathFound = false;

    for (const auto& game : detected)
    {
        QString label = QString("%1 - %2").arg(FilePaths::gameName(game.gameId), game.dataPath);
        combo->addItem(label, QVariant::fromValue(game.dataPath));
        combo->setItemData(combo->count() - 1, QVariant::fromValue(static_cast<int>(game.gameId)), Qt::UserRole + 1);

        if (QDir(game.dataPath) == QDir(dataPath))
        {
            combo->setCurrentIndex(combo->count() - 1);
            currentPathFound = true;
            currentGame = game.gameId;
        }
    }

    if (!currentPathFound && QDir(dataPath).exists())
    {
        QString label = QString("Current - %1").arg(dataPath);
        combo->insertItem(0, label, QVariant::fromValue(dataPath));
        combo->setItemData(0, QVariant::fromValue(static_cast<int>(Game_None)), Qt::UserRole + 1);
        combo->setCurrentIndex(0);
    }
    else if (!currentPathFound && combo->count() > 0)
    {
        combo->insertItem(0, "Select detected game...", QVariant());
        combo->setCurrentIndex(0);
    }
}

void DataDialog::newSelection(const QModelIndex& current, const QModelIndex& previous)
{
    FileInfo info = dataTable->getInfoAtSelected(current);
    authorLineEdit()->setText(info.author);
    descriptionTextEdit()->setPlainText(info.description);

    if (info.flags.test(FileFlag::Master) || info.flags.test(FileFlag::LightMaster))
    {
        authorLineEdit()->setEnabled(false);
        descriptionTextEdit()->setEnabled(false);
        activeButton()->setEnabled(true);
        activeButton()->setText("Set as Active File");
    }
    else
    {
        authorLineEdit()->setEnabled(true);
        descriptionTextEdit()->setEnabled(true);
        activeButton()->setEnabled(true);

        // Show if this file is currently the active one
        if (current.row() == dataTable->getActiveRow())
            activeButton()->setText(QString("Active: %1").arg(info.fileName));
        else
            activeButton()->setText("Set as Active File");
    }

    QFileInfo dateInfo{ dataPath + "/" + info.fileName };
    createdLabel()->setText(
        QString("Created On: %1").arg(
            dateInfo.birthTime().toString("dd/MM/yy hh:mm AP")
        )
    );
    modifiedLabel()->setText(
        QString("Modified On: %1").arg(
            dateInfo.lastModified().toString("dd/MM/yy hh:mm AP")
        )
    );
}

void DataDialog::savePathToConfig()
{
    QString configPath = FilePaths::configFilePath();
    QSettings conf(configPath, QSettings::IniFormat);
    conf.beginGroup("OpenCK");
    conf.setValue("DataDirectory", dataPath);
    conf.setValue("GameId", static_cast<int>(currentGame));
    if (currentGame != Game_None)
        conf.setValue(FilePaths::dataDirKey(currentGame), dataPath);
    conf.endGroup();
}

void DataDialog::configureTable()
{
    dataTable.reset(new DataTable(dataPath));

    // Pre-select active plugins from plugins.txt
    if (currentGame != Game_None)
    {
        QStringList activePlugins = FilePaths::readActivePlugins(currentGame, dataPath);
        LOG_INFO(QString("Found %1 active plugins for game").arg(activePlugins.size()));
        for (const auto& plugin : activePlugins)
        {
            LOG_DEBUG(QString("  Active: %1").arg(plugin));
        }
        if (!activePlugins.isEmpty())
            dataTable->setSelectedFiles(activePlugins);
    }
    else
    {
        // Try all known games to find a matching plugins.txt
        for (int i = 1; i < Game_NumGames; i++)
        {
            GameId gid = static_cast<GameId>(i);
            QStringList activePlugins = FilePaths::readActivePlugins(gid, dataPath);
            if (!activePlugins.isEmpty())
            {
                LOG_INFO(QString("Found %1 active plugins (fallback)").arg(activePlugins.size()));
                dataTable->setSelectedFiles(activePlugins);
                break;
            }
        }
    }

    tableView()->setModel(dataTable.get());
    tableView()->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView()->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView()->verticalHeader()->hide();
    tableView()->verticalHeader()->setDefaultSectionSize(12);
    tableView()->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableView()->horizontalHeader()->setFrameStyle(QFrame::VLine | QFrame::Plain);

    auto selectionModel{ tableView()->selectionModel() };
    connect(selectionModel, &QItemSelectionModel::currentRowChanged,
            this, &DataDialog::newSelection);

    connect(tableView(), &QTableView::doubleClicked,
            dataTable.get(), &DataTable::doubleClicked);
}

void DataDialog::configureList()
{
    mastersList.reset(new MastersList());
    mastersView()->setModel(mastersList.get());
    mastersView()->setSelectionMode(QAbstractItemView::NoSelection);
    mastersView()->setStyleSheet(
        "QListView::item:!selected{ border-bottom: 1px solid #CDCDCD; padding: 2px; }"
    );

    connect(dataTable.get(), &DataTable::newFileSelected,
            mastersList.get(), &MastersList::update);
}

void DataDialog::refreshDataTable()
{
    configureTable();
    configureList();
}

void DataDialog::accept()
{
    auto files = dataTable->getFiles();
    
    QStringList fileNames = std::get<0>(files);
    int active = std::get<1>(files);
    bool isNew = false;
    QString savePath;

    if (fileNames.isEmpty())
    {
        QMessageBox::warning(this, "No Files Selected", "No plugin files are selected. Please select at least one file.");
        return;
    }

    if (active == -1)
    {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "No Active Plugin",
            "No active plugin is set. The editor will open in read-only mode.\n\nContinue anyway?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );

        if (reply != QMessageBox::Yes)
            return;

        isNew = true;
        savePath = "";
    }
    else
    {
        savePath = fileNames.at(active);
    }

    this->close();
    
    emit addDocument(fileNames, savePath, isNew);
}

void DataDialog::on_activeButton_clicked()
{
    QModelIndex idx = tableView()->selectionModel()->currentIndex();
    if (!idx.isValid())
    {
        QMessageBox::information(this, "No Selection", "Select a file first, then click Set as Active.");
        return;
    }

    dataTable->setActive(idx);

    FileInfo info = dataTable->getInfoAtSelected(idx);
    activeButton()->setText(QString("Active: %1").arg(info.fileName));
    LOG_INFO(QString("Set active file: %1").arg(info.fileName));
}

void DataDialog::on_gameSelector_currentIndexChanged(int index)
{
    if (index <= 0)
        return;

    QComboBox* combo = gameSelector();
    QString path = combo->currentData().toString();
    int gameIdInt = combo->itemData(index, Qt::UserRole + 1).toInt();
    GameId gameId = static_cast<GameId>(gameIdInt);

    if (!path.isEmpty() && QDir(path).exists())
    {
        dataPath = path;
        currentGame = gameId;
        savePathToConfig();
        refreshDataTable();
    }
}

void DataDialog::on_browseButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Game Data Directory",
        dataPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!dir.isEmpty())
    {
        dataPath = dir;
        currentGame = Game_None;
        savePathToConfig();

        // Add to combo box
        QComboBox* combo = gameSelector();
        QString label = QString("Custom - %1").arg(dataPath);
        combo->insertItem(0, label, QVariant::fromValue(dataPath));
        combo->setItemData(0, QVariant::fromValue(static_cast<int>(Game_None)), Qt::UserRole + 1);
        combo->setCurrentIndex(0);

        refreshDataTable();
    }
}

QTableView* DataDialog::tableView()
{
    return ui->dataTableView;
}

QLineEdit* DataDialog::authorLineEdit()
{
    return ui->authorLineEdit;
}

QPlainTextEdit* DataDialog::descriptionTextEdit()
{
    return ui->descriptionTextEdit;
}

QListView* DataDialog::mastersView()
{
    return ui->mastersListView;
}

QLabel* DataDialog::createdLabel()
{
    return ui->createdLabel;
}

QLabel* DataDialog::modifiedLabel()
{
    return ui->modifiedLabel;
}

QPushButton* DataDialog::activeButton()
{
    return ui->activeButton;
}

QComboBox* DataDialog::gameSelector()
{
    return ui->gameSelector;
}

QPushButton* DataDialog::browseButton()
{
    return ui->browseButton;
}
