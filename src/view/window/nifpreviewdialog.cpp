#include "nifpreviewdialog.hpp"

#include <QDir>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QProcess>
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include "../../model/tools/blenderlauncher.hpp"

NifPreviewDialog::NifPreviewDialog(const QString& nifPath, QWidget* parent)
    : QDialog(parent)
    , m_previewLabel(nullptr)
    , m_nifPath(nifPath)
    , m_currentIndex(0)
    , m_zoomLevel(1.0)
    , pageLabel(nullptr)
    , zoomLabel(nullptr)
    , previousButton(nullptr)
    , nextButton(nullptr)
    , zoomInButton(nullptr)
    , zoomOutButton(nullptr)
    , fitButton(nullptr)
    , actualSizeButton(nullptr)
{
    setWindowTitle(tr("NIF Preview - %1").arg(QFileInfo(nifPath).fileName()));
    setMinimumSize(600, 500);

    auto* mainLayout = new QVBoxLayout(this);

    m_previewLabel = new QLabel(this);
    m_previewLabel->setMinimumSize(400, 400);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setText(tr("Loading preview..."));
    mainLayout->addWidget(m_previewLabel);

    auto* controlLayout = new QHBoxLayout();

    auto* navLayout = new QVBoxLayout();
    previousButton = new QPushButton(tr("Previous"), this);
    nextButton = new QPushButton(tr("Next"), this);
    navLayout->addWidget(previousButton);
    navLayout->addWidget(nextButton);
    connect(previousButton, &QPushButton::clicked, this, &NifPreviewDialog::onPreviousClicked);
    connect(nextButton, &QPushButton::clicked, this, &NifPreviewDialog::onNextClicked);
    controlLayout->addLayout(navLayout);

    auto* zoomLayout = new QVBoxLayout();
    zoomInButton = new QPushButton(tr("Zoom In"), this);
    zoomOutButton = new QPushButton(tr("Zoom Out"), this);
    fitButton = new QPushButton(tr("Fit to Window"), this);
    actualSizeButton = new QPushButton(tr("Actual Size"), this);
    zoomLayout->addWidget(zoomInButton);
    zoomLayout->addWidget(zoomOutButton);
    zoomLayout->addWidget(fitButton);
    zoomLayout->addWidget(actualSizeButton);
    connect(zoomInButton, &QPushButton::clicked, this, &NifPreviewDialog::onZoomInClicked);
    connect(zoomOutButton, &QPushButton::clicked, this, &NifPreviewDialog::onZoomOutClicked);
    connect(fitButton, &QPushButton::clicked, this, &NifPreviewDialog::onFitToWindowClicked);
    connect(actualSizeButton, &QPushButton::clicked, this, &NifPreviewDialog::onActualSizeClicked);
    controlLayout->addLayout(zoomLayout);

    mainLayout->addLayout(controlLayout);

    pageLabel = new QLabel(this);
    mainLayout->addWidget(pageLabel);

    zoomLabel = new QLabel(this);
    mainLayout->addWidget(zoomLabel);

    QFileInfo nifInfo(nifPath);
    m_previewDir = nifInfo.dir().absoluteFilePath(".cache/preview_" + nifInfo.baseName());
    QDir().mkpath(m_previewDir);

    generatePreviews();

    if (!m_previewFiles.isEmpty()) {
        loadPreview(0);
    } else {
        m_previewLabel->setText(tr("No previews available.\nEnsure Blender is installed and configured."));
        updateNavigationButtons();
        updateZoomLabel();
    }
}

NifPreviewDialog::~NifPreviewDialog() = default;

void NifPreviewDialog::generatePreviews()
{
    QDir previewDir(m_previewDir);
    QStringList filters;
    filters << "preview_*.png";
    m_previewFiles = previewDir.entryList(filters, QDir::Files, QDir::Name);

    if (!m_previewFiles.isEmpty()) {
        return;
    }

    QString scriptPath = QApplication::applicationDirPath() + "/scripts/blender/preview_generator.py";

    if (!QFile::exists(scriptPath)) {
        return;
    }

    QString blenderPath = BlenderLauncher::getRecommendedBlenderPath();
    if (blenderPath.isEmpty()) {
        return;
    }
    m_previewLabel->setText(tr("Generating previews..."));

    auto* process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, previewDir, process](int exitCode, QProcess::ExitStatus) {
        process->deleteLater();
        if (exitCode == 0) {
            QStringList filters;
            filters << "*.png" << "*.jpg" << "*.bmp";
            QDir dir(previewDir);
            m_previewFiles = dir.entryList(filters, QDir::Files, QDir::Name);
            if (!m_previewFiles.isEmpty()) {
                loadPreview(0);
            } else {
                m_previewLabel->setText(tr("No previews generated."));
            }
        } else {
            m_previewLabel->setText(tr("Preview generation failed."));
        }
    });

    process->start(blenderPath, QStringList()
        << "--background"
        << "--python"
        << scriptPath
        << "--"
        << m_nifPath
        << m_previewDir
        << "256"
        << "256"
        << "4");
}

void NifPreviewDialog::loadPreview(int index)
{
    if (index < 0 || index >= m_previewFiles.size()) {
        return;
    }

    m_currentIndex = index;

    QString previewPath = QDir(m_previewDir).absoluteFilePath(m_previewFiles[index]);
    QPixmap pixmap(previewPath);

    if (!pixmap.isNull()) {
        if (m_zoomLevel != 1.0) {
            pixmap = pixmap.scaled(pixmap.size() * m_zoomLevel, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        m_previewLabel->setPixmap(pixmap);
        m_previewLabel->setAlignment(Qt::AlignCenter);
    }

    updateNavigationButtons();
    updateZoomLabel();
}

void NifPreviewDialog::updateNavigationButtons()
{
    if (previousButton) {
        previousButton->setEnabled(m_currentIndex > 0);
    }
    if (nextButton) {
        nextButton->setEnabled(m_currentIndex < m_previewFiles.size() - 1);
    }
    if (pageLabel) {
        pageLabel->setText(tr("Image %1 of %2").arg(m_currentIndex + 1).arg(m_previewFiles.size()));
    }
}

void NifPreviewDialog::updateZoomLabel()
{
    if (zoomLabel) {
        zoomLabel->setText(tr("Zoom: %1%").arg(int(m_zoomLevel * 100)));
    }
}

void NifPreviewDialog::onPreviousClicked()
{
    if (m_currentIndex > 0) {
        loadPreview(m_currentIndex - 1);
    }
}

void NifPreviewDialog::onNextClicked()
{
    if (m_currentIndex < m_previewFiles.size() - 1) {
        loadPreview(m_currentIndex + 1);
    }
}

void NifPreviewDialog::onZoomInClicked()
{
    m_zoomLevel = qMin(m_zoomLevel * 1.25, 5.0);
    loadPreview(m_currentIndex);
}

void NifPreviewDialog::onZoomOutClicked()
{
    m_zoomLevel = qMax(m_zoomLevel / 1.25, 0.25);
    loadPreview(m_currentIndex);
}

void NifPreviewDialog::onFitToWindowClicked()
{
    QSize windowSize = size() - QSize(50, 50);
    if (m_currentIndex < 0 || m_currentIndex >= m_previewFiles.size()) {
        return;
    }
    QString previewPath = QDir(m_previewDir).absoluteFilePath(m_previewFiles[m_currentIndex]);
    QPixmap pixmap(previewPath);
    if (!pixmap.isNull()) {
        QSize scaledSize = pixmap.scaled(windowSize, Qt::KeepAspectRatio, Qt::SmoothTransformation).size();
        m_zoomLevel = static_cast<double>(scaledSize.width()) / pixmap.width();
        m_previewLabel->setPixmap(pixmap.scaled(windowSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        updateZoomLabel();
    }
}

void NifPreviewDialog::onActualSizeClicked()
{
    m_zoomLevel = 1.0;
    loadPreview(m_currentIndex);
}