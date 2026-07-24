#include "nifcomparisondialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>
#include <QPixmap>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QProcess>
#include <QDir>
#include <QApplication>
#include "../../libs/files/log/logger.hpp"
#include "../../model/tools/blenderlauncher.hpp"

NifComparisonDialog::NifComparisonDialog(const QString& nif1Path, const QString& nif2Path, QWidget* parent)
    : QDialog(parent)
    , m_nif1Label(nullptr)
    , m_nif2Label(nullptr)
    , m_nif1Preview(nullptr)
    , m_nif2Preview(nullptr)
    , m_resultsLabel(nullptr)
    , m_selectNif1Button(nullptr)
    , m_selectNif2Button(nullptr)
    , m_compareButton(nullptr)
    , m_closeButton(nullptr)
    , m_nif1Path(nif1Path)
    , m_nif2Path(nif2Path)
{
    setWindowTitle(tr("NIF Comparison"));
    setMinimumSize(800, 600);

    // Create main layout
    auto* mainLayout = new QVBoxLayout(this);

    // NIF 1 selection
    auto* nif1Layout = new QHBoxLayout();
    m_nif1Label = new QLabel(tr("NIF File 1:"));
    m_selectNif1Button = new QPushButton(tr("Select..."));
    nif1Layout->addWidget(m_nif1Label);
    nif1Layout->addWidget(m_selectNif1Button);
    mainLayout->addLayout(nif1Layout);

    // NIF 2 selection
    auto* nif2Layout = new QHBoxLayout();
    m_nif2Label = new QLabel(tr("NIF File 2:"));
    m_selectNif2Button = new QPushButton(tr("Select..."));
    nif2Layout->addWidget(m_nif2Label);
    nif2Layout->addWidget(m_selectNif2Button);
    mainLayout->addLayout(nif2Layout);

    // Compare button
    m_compareButton = new QPushButton(tr("Compare"));
    mainLayout->addWidget(m_compareButton);

    // Preview area
    auto* previewLayout = new QHBoxLayout();
    
    auto* nif1PreviewLayout = new QVBoxLayout();
    nif1PreviewLayout->addWidget(new QLabel(tr("NIF 1 Preview:")));
    m_nif1Preview = new QLabel();
    m_nif1Preview->setMinimumSize(300, 300);
    m_nif1Preview->setAlignment(Qt::AlignCenter);
    m_nif1Preview->setText(tr("No preview"));
    nif1PreviewLayout->addWidget(m_nif1Preview);
    
    auto* nif2PreviewLayout = new QVBoxLayout();
    nif2PreviewLayout->addWidget(new QLabel(tr("NIF 2 Preview:")));
    m_nif2Preview = new QLabel();
    m_nif2Preview->setMinimumSize(300, 300);
    m_nif2Preview->setAlignment(Qt::AlignCenter);
    m_nif2Preview->setText(tr("No preview"));
    nif2PreviewLayout->addWidget(m_nif2Preview);

    previewLayout->addLayout(nif1PreviewLayout);
    previewLayout->addLayout(nif2PreviewLayout);
    mainLayout->addLayout(previewLayout);

    // Results area
    m_resultsLabel = new QLabel(tr("Results will appear here"));
    m_resultsLabel->setWordWrap(true);
    m_resultsLabel->setMinimumHeight(100);
    mainLayout->addWidget(m_resultsLabel);

    // Close button
    m_closeButton = new QPushButton(tr("Close"));
    mainLayout->addWidget(m_closeButton);

    // Connect signals
    connect(m_selectNif1Button, &QPushButton::clicked, this, &NifComparisonDialog::onNif1Selected);
    connect(m_selectNif2Button, &QPushButton::clicked, this, &NifComparisonDialog::onNif2Selected);
    connect(m_compareButton, &QPushButton::clicked, this, &NifComparisonDialog::onCompareClicked);
    connect(m_closeButton, &QPushButton::clicked, this, &NifComparisonDialog::onCloseClicked);

    // Set initial paths if provided
    if (!nif1Path.isEmpty()) {
        m_nif1Label->setText(tr("NIF File 1: %1").arg(nif1Path));
    }
    
    if (!nif2Path.isEmpty()) {
        m_nif2Label->setText(tr("NIF File 2: %1").arg(nif2Path));
    }
}

void NifComparisonDialog::onNif1Selected()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Select NIF File 1"), "", "NIF Files (*.nif)");
    if (!filePath.isEmpty()) {
        m_nif1Path = filePath;
        m_nif1Label->setText(tr("NIF File 1: %1").arg(filePath));
    }
}

void NifComparisonDialog::onNif2Selected()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Select NIF File 2"), "", "NIF Files (*.nif)");
    if (!filePath.isEmpty()) {
        m_nif2Path = filePath;
        m_nif2Label->setText(tr("NIF File 2: %1").arg(filePath));
    }
}

void NifComparisonDialog::onCompareClicked()
{
    if (m_nif1Path.isEmpty() || m_nif2Path.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select both NIF files to compare"));
        return;
    }

    if (!QFile::exists(m_nif1Path) || !QFile::exists(m_nif2Path)) {
        QMessageBox::warning(this, tr("Error"), tr("One or both NIF files do not exist"));
        return;
    }

    loadComparison(m_nif1Path, m_nif2Path);
}

void NifComparisonDialog::onCloseClicked()
{
    close();
}

void NifComparisonDialog::loadComparison(const QString& nif1Path, const QString& nif2Path)
{
    // Generate previews for both NIFs
    QFileInfo nifInfo1(nif1Path);
    QFileInfo nifInfo2(nif2Path);

    QString previewDir1 = nifInfo1.dir().absoluteFilePath(".cache/preview_" + nifInfo1.baseName());
    QString previewDir2 = nifInfo2.dir().absoluteFilePath(".cache/preview_" + nifInfo2.baseName());

    QDir().mkpath(previewDir1);
    QDir().mkpath(previewDir2);

    // Generate previews using Blender script
    QString scriptPath = QApplication::applicationDirPath() + "/scripts/blender/preview_generator.py";
    
    if (QFile::exists(scriptPath)) {
        QString blenderPath = BlenderLauncher::getRecommendedBlenderPath();
        if (blenderPath.isEmpty()) {
            m_resultsLabel->setText(tr("Blender not found. Please install Blender."));
            return;
        }
        
        auto* process1 = new QProcess(this);
        auto* process2 = new QProcess(this);

        connect(process2, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, process2](int exitCode, QProcess::ExitStatus) {
            process2->deleteLater();
            if (exitCode != 0) {
                LOG_WARNING("Blender preview generation failed for second NIF (exit code: " + QString::number(exitCode) + ")");
            }
            displayResults();
        });

        connect(process1, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, process1, process2, blenderPath, scriptPath, nif2Path, previewDir2](int exitCode, QProcess::ExitStatus) {
            process1->deleteLater();
            if (exitCode != 0) {
                LOG_WARNING("Blender preview generation failed for first NIF (exit code: " + QString::number(exitCode) + ")");
            }
            process2->start(blenderPath, QStringList()
                << "--background"
                << "--python"
                << scriptPath
                << "--"
                << nif2Path
                << previewDir2
                << "256"
                << "256"
                << "4");
        });

        process1->start(blenderPath, QStringList() 
            << "--background" 
            << "--python" 
            << scriptPath 
            << "--" 
            << nif1Path 
            << previewDir1
            << "256"
            << "256"
            << "4");
    } else {
        displayResults();
    }

    // Load previews
    QDir previewDir1Dir(previewDir1);
    QStringList filters;
    filters << "preview_*.png";
    QStringList previewFiles1 = previewDir1Dir.entryList(filters, QDir::Files, QDir::Name);

    if (!previewFiles1.isEmpty()) {
        QString previewPath = QDir(previewDir1).absoluteFilePath(previewFiles1[0]);
        QPixmap pixmap(previewPath);
        if (!pixmap.isNull()) {
            m_nif1Preview->setPixmap(pixmap.scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }

    QDir previewDir2Dir(previewDir2);
    QStringList previewFiles2 = previewDir2Dir.entryList(filters, QDir::Files, QDir::Name);

    if (!previewFiles2.isEmpty()) {
        QString previewPath = QDir(previewDir2).absoluteFilePath(previewFiles2[0]);
        QPixmap pixmap(previewPath);
        if (!pixmap.isNull()) {
            m_nif2Preview->setPixmap(pixmap.scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }

    displayResults();
}

void NifComparisonDialog::displayResults()
{
    QString results;
    QTextStream stream(&results);

    // Load NIF data using PyNifly
    QString scriptPath = QApplication::applicationDirPath() + "/scripts/blender/nif_batch.py";
    
    // For now, display basic file info
    QFileInfo nifInfo1(m_nif1Path);
    QFileInfo nifInfo2(m_nif2Path);

    stream << "<h3>Comparison Results</h3>";
    stream << "<table>";
    stream << "<tr><td><b>File 1:</b></td><td>" << nifInfo1.fileName() << "</td></tr>";
    stream << "<tr><td><b>File 2:</b></td><td>" << nifInfo2.fileName() << "</td></tr>";
    stream << "<tr><td><b>Size 1:</b></td><td>" << nifInfo1.size() << " bytes</td></tr>";
    stream << "<tr><td><b>Size 2:</b></td><td>" << nifInfo2.size() << " bytes</td></tr>";
    stream << "<tr><td><b>Modified 1:</b></td><td>" << nifInfo1.lastModified().toString() << "</td></tr>";
    stream << "<tr><td><b>Modified 2:</b></td><td>" << nifInfo2.lastModified().toString() << "</td></tr>";
    stream << "</table>";

    m_resultsLabel->setText(results);
}
