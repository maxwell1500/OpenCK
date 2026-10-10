#include "modmanagerdialog.hpp"
#include "../../model/tools/modmanagerdetection.hpp"
#include "../../../libs/files/filepaths.hpp"
#include "logger.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QHeaderView>
#include <QDir>
#include <QTableWidgetItem>

ModManagerDialog::ModManagerDialog(QWidget* parent)
    : QDialog(parent)
{
    LOG_DEBUG("ModManagerDialog created");
    setWindowTitle("Mod Manager Integration");
    setMinimumSize(550, 500);
    setupUI();
    populateUI();
}

ModManagerDialog::~ModManagerDialog()
{
    LOG_DEBUG("ModManagerDialog destroyed");
}

void ModManagerDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Detected Mod Manager", this);
    auto* infoLayout = new QGridLayout(infoGroup);

    infoLayout->addWidget(new QLabel("Manager:", infoGroup), 0, 0);
    managerStatusLabel = new QLabel("None", infoGroup);
    managerStatusLabel->setStyleSheet("font-weight: bold;");
    infoLayout->addWidget(managerStatusLabel, 0, 1);

    infoLayout->addWidget(new QLabel("Install Path:", infoGroup), 1, 0);
    installPathLabel = new QLabel("-", infoGroup);
    installPathLabel->setWordWrap(true);
    infoLayout->addWidget(installPathLabel, 1, 1);

    infoLayout->addWidget(new QLabel("Version:", infoGroup), 2, 0);
    versionLabel = new QLabel("-", infoGroup);
    infoLayout->addWidget(versionLabel, 2, 1);

    infoLayout->addWidget(new QLabel("Game Path:", infoGroup), 3, 0);
    gamePathLabel = new QLabel("-", infoGroup);
    gamePathLabel->setWordWrap(true);
    infoLayout->addWidget(gamePathLabel, 3, 1);

    infoLayout->addWidget(new QLabel("Status:", infoGroup), 4, 0);
    statusLabel = new QLabel("-", infoGroup);
    infoLayout->addWidget(statusLabel, 4, 1);

    mainLayout->addWidget(infoGroup);

    auto* profileGroup = new QGroupBox("Profile", this);
    auto* profileLayout = new QHBoxLayout(profileGroup);
    profileCombo = new QComboBox(profileGroup);
    profileLayout->addWidget(new QLabel("Profile:", profileGroup));
    profileLayout->addWidget(profileCombo, 1);
    mainLayout->addWidget(profileGroup);

    auto* deploymentGroup = new QGroupBox("Deployed Files (Vortex)", this);
    auto* deploymentLayout = new QGridLayout(deploymentGroup);

    deploymentLayout->addWidget(new QLabel("Manifest:", deploymentGroup), 0, 0);
    deploymentStatusLabel = new QLabel("-", deploymentGroup);
    deploymentStatusLabel->setWordWrap(true);
    deploymentLayout->addWidget(deploymentStatusLabel, 0, 1);

    deploymentLayout->addWidget(new QLabel("Method:", deploymentGroup), 1, 0);
    deploymentMethodLabel = new QLabel("-", deploymentGroup);
    deploymentLayout->addWidget(deploymentMethodLabel, 1, 1);

    deploymentTable = new QTableWidget(deploymentGroup);
    deploymentTable->setColumnCount(4);
    deploymentTable->setHorizontalHeaderLabels(
        { "Deployed Path", "Source Mod", "Staged Copy", "Link" });
    deploymentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    deploymentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    deploymentTable->horizontalHeader()->setSectionResizeMode(
        3, QHeaderView::ResizeToContents);
    deploymentLayout->addWidget(deploymentTable, 2, 0, 1, 2);

    auto* resolveLayout = new QHBoxLayout();
    resolveLayout->addWidget(new QLabel("Resolve asset:", deploymentGroup));
    assetPathEdit = new QLineEdit(deploymentGroup);
    assetPathEdit->setPlaceholderText("meshes/chair.nif");
    resolveLayout->addWidget(assetPathEdit, 1);
    auto* resolveButton = new QPushButton("Resolve", deploymentGroup);
    resolveLayout->addWidget(resolveButton);
    deploymentLayout->addLayout(resolveLayout, 3, 0, 1, 2);
    assetResultLabel = new QLabel("-", deploymentGroup);
    assetResultLabel->setWordWrap(true);
    deploymentLayout->addWidget(assetResultLabel, 4, 0, 1, 2);

    mainLayout->addWidget(deploymentGroup);

    auto* modsGroup = new QGroupBox("Installed Mods", this);
    auto* modsLayout = new QVBoxLayout(modsGroup);
    modListWidget = new QListWidget(modsGroup);
    modListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    modsLayout->addWidget(modListWidget);
    mainLayout->addWidget(modsGroup);

    auto* buttonLayout = new QHBoxLayout();
    refreshButton = new QPushButton("Refresh Detection", this);
    openManagerButton = new QPushButton("Open in Mod Manager", this);
    launchProfileButton = new QPushButton("Launch with Profile", this);
    auto* closeButton = new QPushButton("Close", this);

    openManagerButton->setEnabled(false);
    launchProfileButton->setEnabled(false);

    buttonLayout->addWidget(refreshButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(openManagerButton);
    buttonLayout->addWidget(launchProfileButton);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    connect(refreshButton, &QPushButton::clicked, this, &ModManagerDialog::onRefreshDetection);
    connect(openManagerButton, &QPushButton::clicked, this, &ModManagerDialog::onOpenModManager);
    connect(launchProfileButton, &QPushButton::clicked, this, &ModManagerDialog::onLaunchWithProfile);
    connect(resolveButton, &QPushButton::clicked, this, &ModManagerDialog::onResolveAsset);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
}

void ModManagerDialog::populateUI()
{
    auto info = ModManagerDetection::detect();

    if (info.type == ModManagerDetection::ModManager::None)
    {
        managerStatusLabel->setText("Not Detected");
        managerStatusLabel->setStyleSheet("font-weight: bold; color: red;");
        installPathLabel->setText("-");
        versionLabel->setText("-");
        gamePathLabel->setText("-");
        statusLabel->setText("No mod manager found");
        openManagerButton->setEnabled(false);
        launchProfileButton->setEnabled(false);
        return;
    }

    QString managerName;
    if (info.type == ModManagerDetection::ModManager::MO2)
    {
        managerName = "Mod Organizer 2";
    }
    else if (info.type == ModManagerDetection::ModManager::Vortex)
    {
        managerName = "Vortex";
    }
    else
    {
        managerName = "Unknown";
    }

    managerStatusLabel->setText(managerName);
    managerStatusLabel->setStyleSheet("font-weight: bold; color: green;");
    installPathLabel->setText(info.installPath);
    versionLabel->setText(info.version.isEmpty() ? "-" : info.version);
    gamePathLabel->setText(info.gamePath.isEmpty() ? "-" : info.gamePath);

    if (info.isRunning)
    {
        statusLabel->setText("Running");
        statusLabel->setStyleSheet("color: green;");
    }
    else
    {
        statusLabel->setText("Stopped");
        statusLabel->setStyleSheet("color: red;");
    }

    profileCombo->clear();
    if (!info.profiles.isEmpty())
    {
        profileCombo->addItems(info.profiles);
        QString currentProfile = ModManagerDetection::getMO2Profile();
        if (!currentProfile.isEmpty())
        {
            int idx = profileCombo->findText(currentProfile);
            if (idx >= 0)
            {
                profileCombo->setCurrentIndex(idx);
            }
        }
    }

    modListWidget->clear();
    QStringList mods = ModManagerDetection::getInstalledMods(info.type);
    for (const QString& mod : mods)
    {
        modListWidget->addItem(mod);
    }

    openManagerButton->setEnabled(true);
    launchProfileButton->setEnabled(true);

    // Deployment state lives in the game's own data directory, independent
    // of whether the manager itself is detected. The manager's own gamePath
    // wins; otherwise fall back to OpenCK's configured data directory.
    QString dataDir = info.gamePath;
    if (dataDir.isEmpty())
    {
        QSettings conf(FilePaths::configFilePath(), QSettings::IniFormat);
        conf.beginGroup(QStringLiteral("OpenCK"));
        dataDir = conf.value(QStringLiteral("DataDirectory")).toString();
        conf.endGroup();
    }
    populateDeployment(dataDir);
}

void ModManagerDialog::populateDeployment(const QString& gameDataDir)
{
    mDeployment = ModDeploymentResolver::VortexDeployment();
    deploymentTable->setRowCount(0);
    assetResultLabel->setText("-");

    if (gameDataDir.isEmpty())
    {
        deploymentStatusLabel->setText("No game data directory known");
        deploymentMethodLabel->setText("-");
        return;
    }

    const QString manifestPath =
        ModDeploymentResolver::findVortexDeployment(gameDataDir);
    if (manifestPath.isEmpty())
    {
        deploymentStatusLabel->setText(
            QString("No vortex.deployment.json in %1").arg(
                QDir::toNativeSeparators(gameDataDir)));
        deploymentMethodLabel->setText("-");
        return;
    }

    if (!ModDeploymentResolver::readVortexDeployment(manifestPath, mDeployment))
    {
        deploymentStatusLabel->setText("Manifest unreadable");
        deploymentMethodLabel->setText("-");
        return;
    }

    deploymentStatusLabel->setText(
        QString("%1 deployed file(s), game %2")
            .arg(mDeployment.files.size())
            .arg(mDeployment.gameId.isEmpty() ? QStringLiteral("?")
                                              : mDeployment.gameId));
    deploymentMethodLabel->setText(
        mDeployment.deploymentMethod.isEmpty()
            ? QStringLiteral("(unknown)")
            : mDeployment.deploymentMethod);

    deploymentTable->setRowCount(mDeployment.files.size());
    for (int i = 0; i < mDeployment.files.size(); ++i)
    {
        const ModDeploymentResolver::DeployedFile& file = mDeployment.files.at(i);
        const QString staged =
            ModDeploymentResolver::sourceFilePath(mDeployment, file.relPath);
        const QString deployed =
            ModDeploymentResolver::deployedFilePath(mDeployment, file.relPath);
        const QString link = staged.isEmpty() || deployed.isEmpty()
            ? QString()
            : ModDeploymentResolver::linkKindName(
                  ModDeploymentResolver::linkStatus(staged, deployed));

        deploymentTable->setItem(
            i, 0, new QTableWidgetItem(QDir::toNativeSeparators(file.relPath)));
        deploymentTable->setItem(i, 1, new QTableWidgetItem(file.source));
        deploymentTable->setItem(
            i, 2, new QTableWidgetItem(staged.isEmpty() ? QStringLiteral("-") : staged));
        deploymentTable->setItem(
            i, 3, new QTableWidgetItem(link.isEmpty() ? QStringLiteral("-") : link));
    }
    deploymentTable->resizeColumnsToContents();
}

void ModManagerDialog::onResolveAsset()
{
    const QString rel = assetPathEdit->text().trimmed();
    if (rel.isEmpty())
    {
        assetResultLabel->setText("Enter a relative asset path first.");
        return;
    }

    if (!mDeployment.isValid())
    {
        assetResultLabel->setText("No Vortex deployment loaded.");
        return;
    }

    const QString deployed =
        ModDeploymentResolver::deployedFilePath(mDeployment, rel);
    if (deployed.isEmpty())
    {
        assetResultLabel->setText(
            QString("'%1' is not deployed by any mod").arg(rel));
        return;
    }

    const QString staged = ModDeploymentResolver::sourceFilePath(mDeployment, rel);
    QString text = QString("Deployed: %1").arg(deployed);
    if (!staged.isEmpty())
    {
        text += QString("\nStaged: %1 (%2)").arg(
            staged,
            ModDeploymentResolver::linkKindName(
                ModDeploymentResolver::linkStatus(staged, deployed)));
    }
    assetResultLabel->setText(text);
}

void ModManagerDialog::onRefreshDetection()
{
    LOG_INFO("Refreshing mod manager detection");
    populateUI();
}

void ModManagerDialog::onOpenModManager()
{
    auto info = ModManagerDetection::detect();
    if (info.type == ModManagerDetection::ModManager::None)
    {
        QMessageBox::warning(this, "Mod Manager", "No mod manager detected.");
        return;
    }

    QString exePath;
    if (info.type == ModManagerDetection::ModManager::MO2)
    {
        exePath = info.installPath + "/ModOrganizer.exe";
    }
    else if (info.type == ModManagerDetection::ModManager::Vortex)
    {
        exePath = info.installPath + "/Vortex.exe";
    }

    if (!QFile::exists(exePath))
    {
        QMessageBox::warning(this, "Mod Manager", QString("Executable not found: %1").arg(exePath));
        return;
    }

    LOG_INFO(QString("Launching mod manager: %1").arg(exePath));
    QProcess::startDetached(exePath);
}

void ModManagerDialog::onLaunchWithProfile()
{
    auto info = ModManagerDetection::detect();
    if (info.type == ModManagerDetection::ModManager::None)
    {
        QMessageBox::warning(this, "Mod Manager", "No mod manager detected.");
        return;
    }

    if (profileCombo->currentIndex() < 0)
    {
        QMessageBox::warning(this, "Mod Manager", "No profile selected.");
        return;
    }

    QString selectedProfile = profileCombo->currentText();
    LOG_INFO(QString("Launching with profile: %1").arg(selectedProfile));

    QString exePath;
    if (info.type == ModManagerDetection::ModManager::MO2)
    {
        exePath = info.installPath + "/ModOrganizer.exe";
    }
    else if (info.type == ModManagerDetection::ModManager::Vortex)
    {
        exePath = info.installPath + "/Vortex.exe";
    }

    if (!QFile::exists(exePath))
    {
        QMessageBox::warning(this, "Mod Manager", QString("Executable not found: %1").arg(exePath));
        return;
    }

    QStringList args;
    if (info.type == ModManagerDetection::ModManager::MO2)
    {
        args << "--profile" << selectedProfile;
    }

    LOG_INFO(QString("Launching with args: %1 %2").arg(exePath).arg(args.join(" ")));
    QProcess::startDetached(exePath, args);
}
