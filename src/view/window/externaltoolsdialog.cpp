#include "externaltoolsdialog.hpp"
#include "logger.hpp"

#include <QProcess>
#include <QMessageBox>
#include <QFileDialog>
#include <QGroupBox>

ExternalToolsDialog::ExternalToolsDialog(QWidget* parent)
    : QDialog(parent)
{
    LOG_DEBUG("ExternalToolsDialog created");
    setWindowTitle("External Tools Configuration");
    setMinimumSize(600, 400);
    setupUI();
    loadSettings();
}

ExternalToolsDialog::~ExternalToolsDialog()
{
    LOG_DEBUG("ExternalToolsDialog destroyed");
}

void ExternalToolsDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* nifSkopeGroup = new QGroupBox("NifSkope (NIF Viewer/Editor)", this);
    auto* nifSkopeLayout = new QHBoxLayout(nifSkopeGroup);
    nifSkopePath = new QLineEdit(this);
    nifSkopePath->setPlaceholderText("Path to NifSkope.exe");
    browseNifSkopeButton = new QPushButton("Browse...", this);
    nifSkopeButton = new QPushButton("Launch", this);
    nifSkopeLayout->addWidget(nifSkopePath);
    nifSkopeLayout->addWidget(browseNifSkopeButton);
    nifSkopeLayout->addWidget(nifSkopeButton);
    mainLayout->addWidget(nifSkopeGroup);

    auto* gimpGroup = new QGroupBox("GIMP (Texture Editor)", this);
    auto* gimpLayout = new QHBoxLayout(gimpGroup);
    gimpPath = new QLineEdit(this);
    gimpPath->setPlaceholderText("Path to GIMP.exe");
    browseGimpButton = new QPushButton("Browse...", this);
    gimpButton = new QPushButton("Launch", this);
    gimpLayout->addWidget(gimpPath);
    gimpLayout->addWidget(browseGimpButton);
    gimpLayout->addWidget(gimpButton);
    mainLayout->addWidget(gimpGroup);

    auto* wryeBashGroup = new QGroupBox("Wrye Bash (Load Order Manager)", this);
    auto* wryeBashLayout = new QHBoxLayout(wryeBashGroup);
    wryeBashPath = new QLineEdit(this);
    wryeBashPath->setPlaceholderText("Path to Wrye Bash.exe");
    browseWryeBashButton = new QPushButton("Browse...", this);
    wryeBashButton = new QPushButton("Launch", this);
    wryeBashLayout->addWidget(wryeBashPath);
    wryeBashLayout->addWidget(browseWryeBashButton);
    wryeBashLayout->addWidget(wryeBashButton);
    mainLayout->addWidget(wryeBashGroup);

    auto* tes5EditGroup = new QGroupBox("TES5Edit (Record Editor)", this);
    auto* tes5EditLayout = new QHBoxLayout(tes5EditGroup);
    tes5EditPath = new QLineEdit(this);
    tes5EditPath->setPlaceholderText("Path to TES5Edit.exe");
    browseTes5EditButton = new QPushButton("Browse...", this);
    tes5EditButton = new QPushButton("Launch", this);
    tes5EditLayout->addWidget(tes5EditPath);
    tes5EditLayout->addWidget(browseTes5EditButton);
    tes5EditLayout->addWidget(tes5EditButton);
    mainLayout->addWidget(tes5EditGroup);

    mainLayout->addStretch();

    auto* bottomLayout = new QHBoxLayout();
    saveButton = new QPushButton("Save Settings", this);
    auto* closeButton = new QPushButton("Close", this);
    bottomLayout->addStretch();
    bottomLayout->addWidget(saveButton);
    bottomLayout->addWidget(closeButton);
    mainLayout->addLayout(bottomLayout);

    connect(nifSkopeButton, &QPushButton::clicked, this, &ExternalToolsDialog::onLaunchNifSkope);
    connect(gimpButton, &QPushButton::clicked, this, &ExternalToolsDialog::onLaunchGimp);
    connect(wryeBashButton, &QPushButton::clicked, this, &ExternalToolsDialog::onLaunchWryeBash);
    connect(tes5EditButton, &QPushButton::clicked, this, &ExternalToolsDialog::onLaunchTes5Edit);

    connect(browseNifSkopeButton, &QPushButton::clicked, this, &ExternalToolsDialog::onBrowseNifSkope);
    connect(browseGimpButton, &QPushButton::clicked, this, &ExternalToolsDialog::onBrowseGimp);
    connect(browseWryeBashButton, &QPushButton::clicked, this, &ExternalToolsDialog::onBrowseWryeBash);
    connect(browseTes5EditButton, &QPushButton::clicked, this, &ExternalToolsDialog::onBrowseTes5Edit);

    connect(saveButton, &QPushButton::clicked, this, &ExternalToolsDialog::onSaveSettings);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
}

void ExternalToolsDialog::loadSettings()
{
    QSettings settings("OpenCK", "ExternalTools");
    nifSkopePath->setText(settings.value("NifSkope/path", "").toString());
    gimpPath->setText(settings.value("GIMP/path", "").toString());
    wryeBashPath->setText(settings.value("WryeBash/path", "").toString());
    tes5EditPath->setText(settings.value("TES5Edit/path", "").toString());
}

void ExternalToolsDialog::onSaveSettings()
{
    QSettings settings("OpenCK", "ExternalTools");
    settings.setValue("NifSkope/path", nifSkopePath->text());
    settings.setValue("GIMP/path", gimpPath->text());
    settings.setValue("WryeBash/path", wryeBashPath->text());
    settings.setValue("TES5Edit/path", tes5EditPath->text());
    QMessageBox::information(this, "Settings", "Tool paths saved successfully.");
    LOG_INFO("External tool paths saved");
}

void ExternalToolsDialog::onLaunchNifSkope()
{
    QString path = nifSkopePath->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "NifSkope", "Please set the NifSkope path first.");
        return;
    }
    LOG_INFO(QString("Launching NifSkope: %1").arg(path));
    QProcess::startDetached(path);
}

void ExternalToolsDialog::onLaunchGimp()
{
    QString path = gimpPath->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "GIMP", "Please set the GIMP path first.");
        return;
    }
    LOG_INFO(QString("Launching GIMP: %1").arg(path));
    QProcess::startDetached(path);
}

void ExternalToolsDialog::onLaunchWryeBash()
{
    QString path = wryeBashPath->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "Wrye Bash", "Please set the Wrye Bash path first.");
        return;
    }
    LOG_INFO(QString("Launching Wrye Bash: %1").arg(path));
    QProcess::startDetached(path);
}

void ExternalToolsDialog::onLaunchTes5Edit()
{
    QString path = tes5EditPath->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "TES5Edit", "Please set the TES5Edit path first.");
        return;
    }
    LOG_INFO(QString("Launching TES5Edit: %1").arg(path));
    QProcess::startDetached(path);
}

void ExternalToolsDialog::onBrowseNifSkope()
{
    QString file = QFileDialog::getOpenFileName(this, "Select NifSkope Executable", "", "Executables (*.exe)");
    if (!file.isEmpty()) nifSkopePath->setText(file);
}

void ExternalToolsDialog::onBrowseGimp()
{
    QString file = QFileDialog::getOpenFileName(this, "Select GIMP Executable", "", "Executables (*.exe)");
    if (!file.isEmpty()) gimpPath->setText(file);
}

void ExternalToolsDialog::onBrowseWryeBash()
{
    QString file = QFileDialog::getOpenFileName(this, "Select Wrye Bash Executable", "", "Executables (*.exe)");
    if (!file.isEmpty()) wryeBashPath->setText(file);
}

void ExternalToolsDialog::onBrowseTes5Edit()
{
    QString file = QFileDialog::getOpenFileName(this, "Select TES5Edit Executable", "", "Executables (*.exe)");
    if (!file.isEmpty()) tes5EditPath->setText(file);
}
