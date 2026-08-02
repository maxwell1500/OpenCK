#include "archivebrowserdialog.hpp"

#include "../libs/files/ba2/bsaarchive.hpp"
#include "../libs/files/ba2/ba2archive.hpp"
#include "../libs/files/audio/fuzparser.hpp"
#include "nifviewportwidget.hpp"
#include "voicepreview.hpp"
#include "logger.hpp"

#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImage>
#include <QLineEdit>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <mmsystem.h>
#  pragma comment(lib, "winmm.lib")
#endif

ArchiveBrowserDialog::ArchiveBrowserDialog(const QString& dataDirectory, QWidget* parent)
    : QDialog(parent)
    , mDataDirectory(dataDirectory)
{
    setWindowTitle(tr("Archive Browser"));
    setMinimumSize(720, 560);
    setupUi();
    rebuildList();
    scanDataDirectory();
}

ArchiveBrowserDialog::~ArchiveBrowserDialog()
{
    closeArchive();
}

void ArchiveBrowserDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    auto* splitter = new QSplitter(Qt::Vertical, this);

    auto* topWidget = new QWidget();
    auto* topLayout = new QVBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(4);

    auto* toolbar = new QHBoxLayout();
    mQuickOpen = new QComboBox(this);
    mQuickOpen->setObjectName(QStringLiteral("quickOpen"));
    mQuickOpen->setMinimumWidth(320);
    mQuickOpen->setToolTip(tr("Archives found in the game data directory"));
    mBrowseBtn = new QPushButton(tr("Open Archive..."), this);
    mArchiveLabel = new QLabel(tr("No archive open"), this);
    toolbar->addWidget(mQuickOpen);
    toolbar->addWidget(mBrowseBtn);
    toolbar->addWidget(mArchiveLabel, 1);
    topLayout->addLayout(toolbar);

    auto* filterRow = new QHBoxLayout();
    mFilterCombo = new QComboBox(this);
    mFilterCombo->setObjectName(QStringLiteral("filterCombo"));
    mFilterCombo->addItems({
        tr("All Files"), tr("Models"), tr("Textures"), tr("Sounds"), tr("Voice (.fuz)")
    });
    mSearchEdit = new QLineEdit(this);
    mSearchEdit->setObjectName(QStringLiteral("searchEdit"));
    mSearchEdit->setPlaceholderText(tr("Search entries..."));
    filterRow->addWidget(mFilterCombo);
    filterRow->addWidget(mSearchEdit, 1);
    topLayout->addLayout(filterRow);

    mList = new QListWidget(this);
    mList->setObjectName(QStringLiteral("entryList"));
    mList->setSelectionMode(QAbstractItemView::SingleSelection);
    mList->setUniformItemSizes(true);
    topLayout->addWidget(mList, 1);

    auto* btnRow = new QHBoxLayout();
    mPlayBtn = new QPushButton(tr("Play"), this);
    mExtractBtn = new QPushButton(tr("Extract Selected..."), this);
    mExtractAllBtn = new QPushButton(tr("Extract All..."), this);
    mPlayBtn->setEnabled(false);
    btnRow->addWidget(mPlayBtn);
    btnRow->addWidget(mExtractBtn);
    btnRow->addWidget(mExtractAllBtn);
    btnRow->addStretch();
    topLayout->addLayout(btnRow);

    splitter->addWidget(topWidget);

    auto* bottomWidget = new QWidget();
    auto* bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(0, 0, 0, 0);

    mPreviewImage = new QLabel(this);
    mPreviewImage->setAlignment(Qt::AlignCenter);
    mPreviewImage->setMinimumHeight(120);
    mPreviewInfo = new QLabel(tr("Select an entry to preview."), this);
    mPreviewInfo->setWordWrap(true);
    mPreviewInfo->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bottomLayout->addWidget(mPreviewImage);
    bottomLayout->addWidget(mPreviewInfo);

    splitter->addWidget(bottomWidget);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter);

    connect(mBrowseBtn, &QPushButton::clicked, this, &ArchiveBrowserDialog::browseArchive);
    connect(mQuickOpen, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &ArchiveBrowserDialog::onQuickOpenChanged);
    connect(mFilterCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &ArchiveBrowserDialog::onFilterChanged);
    connect(mSearchEdit, &QLineEdit::textChanged,
            this, &ArchiveBrowserDialog::onSearchTextChanged);
    connect(mList, &QListWidget::currentRowChanged,
            this, &ArchiveBrowserDialog::onEntrySelected);
    connect(mList, &QListWidget::itemDoubleClicked,
            this, &ArchiveBrowserDialog::onEntryDoubleClicked);
    connect(mPlayBtn, &QPushButton::clicked, this, &ArchiveBrowserDialog::playSelected);
    connect(mExtractBtn, &QPushButton::clicked, this, &ArchiveBrowserDialog::extractSelected);
    connect(mExtractAllBtn, &QPushButton::clicked, this, &ArchiveBrowserDialog::extractAll);
}

void ArchiveBrowserDialog::scanDataDirectory()
{
    if (mDataDirectory.isEmpty()) return;

    QStringList archivePaths;
    const QDir dir(mDataDirectory);
    const QStringList filters = { "*.bsa", "*.ba2" };
    const auto files = dir.entryList(filters, QDir::Files, QDir::Name);
    for (const auto& f : files)
        archivePaths << dir.absoluteFilePath(f);

    mQuickOpen->blockSignals(true);
    mQuickOpen->clear();
    for (const auto& p : archivePaths)
        mQuickOpen->addItem(QFileInfo(p).fileName(), p);
    mQuickOpen->blockSignals(false);
    if (!archivePaths.isEmpty())
        setStatus(tr("%1 archive(s) found in %2")
                      .arg(archivePaths.size()).arg(mDataDirectory));
}

void ArchiveBrowserDialog::browseArchive()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Open Archive"),
        mDataDirectory, tr("Bethesda Archives (*.bsa *.ba2);;All Files (*)"));
    if (path.isEmpty()) return;
    openArchive(path);
}

void ArchiveBrowserDialog::onQuickOpenChanged(int index)
{
    if (index < 0) return;
    const QString path = mQuickOpen->itemData(index).toString();
    if (!path.isEmpty()) openArchive(path);
}

void ArchiveBrowserDialog::openArchive(const QString& path)
{
    closeArchive();

    mBsa = new BsaArchive();
    if (mBsa->open(path))
    {
        mKind = Kind::Bsa;
        LOG_INFO(QString("Archive Browser: opened BSA %1 (%2 files)")
                     .arg(path).arg(mBsa->fileCount()));
    }
    else
    {
        delete mBsa;
        mBsa = nullptr;
        mBa2 = new Ba2Archive();
        if (mBa2->open(path))
        {
            mKind = Kind::Ba2;
            LOG_INFO(QString("Archive Browser: opened BA2 %1 (%2 files)")
                         .arg(path).arg(mBa2->fileCount()));
        }
        else
        {
            delete mBa2;
            mBa2 = nullptr;
            mKind = Kind::None;
            QMessageBox::critical(this, tr("Error"),
                tr("Failed to open archive:\n%1").arg(path));
            rebuildList();
            return;
        }
    }

    mArchiveLabel->setText(tr("%1 (%2 files)")
                               .arg(mKind == Kind::Bsa ? mBsa->name() : mBa2->name())
                               .arg(entryCount()));
    setWindowTitle(tr("Archive Browser - %1")
                       .arg(mKind == Kind::Bsa ? mBsa->name() : mBa2->name()));
    rebuildList();
}

void ArchiveBrowserDialog::closeArchive()
{
    delete mBsa;
    mBsa = nullptr;
    delete mBa2;
    mBa2 = nullptr;
    mKind = Kind::None;
    mVisible.clear();
    mSelectedIndex = -1;
}

int ArchiveBrowserDialog::entryCount() const
{
    switch (mKind)
    {
    case Kind::Bsa: return mBsa ? mBsa->fileCount() : 0;
    case Kind::Ba2: return mBa2 ? static_cast<int>(mBa2->fileCount()) : 0;
    default: return 0;
    }
}

QString ArchiveBrowserDialog::entryPath(int index) const
{
    switch (mKind)
    {
    case Kind::Bsa:
        if (mBsa && index >= 0 && index < mBsa->entries().size())
            return mBsa->entries().at(index).fullPath;
        break;
    case Kind::Ba2:
        if (mBa2 && index >= 0 && index < static_cast<int>(mBa2->entries().size()))
            return mBa2->entries().at(index).relativePath;
        break;
    default: break;
    }
    return QString();
}

bool ArchiveBrowserDialog::readEntry(int index, QByteArray& out) const
{
    if (mKind == Kind::Bsa)
        return mBsa && mBsa->readData(static_cast<quint32>(index), out);
    if (mKind == Kind::Ba2 && mBa2)
    {
        QString tmp = QDir::tempPath() + QStringLiteral("/openck_ba2_")
            + QString::number(index) + QStringLiteral(".tmp");
        if (!mBa2->extract(static_cast<quint32>(index), tmp)) return false;
        QFile f(tmp);
        const bool ok = f.open(QIODevice::ReadOnly);
        if (ok) out = f.readAll();
        f.close();
        QFile::remove(tmp);
        return ok;
    }
    return false;
}

bool ArchiveBrowserDialog::extractToTemp(int index, QString& tmpPath) const
{
    tmpPath = QDir::tempPath() + QStringLiteral("/openck_arc_")
        + QString::number(index) + QStringLiteral("_")
        + QFileInfo(entryPath(index)).fileName();
    if (mKind == Kind::Bsa)
        return mBsa && mBsa->extract(static_cast<quint32>(index), tmpPath);
    if (mKind == Kind::Ba2)
        return mBa2 && mBa2->extract(static_cast<quint32>(index), tmpPath);
    return false;
}

bool ArchiveBrowserDialog::isTextureExt(const QString& lowerPath)
{
    static const QStringList exts = {
        QStringLiteral("dds"), QStringLiteral("tga"), QStringLiteral("bmp"),
        QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
        QStringLiteral("exr")
    };
    return exts.contains(QFileInfo(lowerPath).suffix());
}

bool ArchiveBrowserDialog::isVisibleByFilter(const QString& lowerPath, int filterIndex)
{
    const QString ext = QFileInfo(lowerPath).suffix();
    switch (filterIndex)
    {
    case 1: return ext == QStringLiteral("nif");
    case 2: return isTextureExt(lowerPath);
    case 3:
        return ext == QStringLiteral("wav") || ext == QStringLiteral("ogg")
            || ext == QStringLiteral("xwm") || ext == QStringLiteral("fuz");
    case 4: return ext == QStringLiteral("fuz");
    default: return true;
    }
}

void ArchiveBrowserDialog::rebuildList()
{
    mVisible.clear();
    mList->clear();
    mSelectedIndex = -1;
    clearPreview();

    if (mKind == Kind::None)
    {
        setStatus(tr("Open an archive to browse its contents."));
        mArchiveLabel->setText(tr("No archive open"));
        mPlayBtn->setEnabled(false);
        return;
    }

    const int filterIndex = mFilterCombo ? mFilterCombo->currentIndex() : 0;
    const QString search = mSearchEdit ? mSearchEdit->text().trimmed().toLower() : QString();

    for (int i = 0; i < entryCount(); ++i)
    {
        const QString path = entryPath(i);
        const QString lower = path.toLower();
        if (!isVisibleByFilter(lower, filterIndex)) continue;
        if (!search.isEmpty() && !lower.contains(search)) continue;

        auto* item = new QListWidgetItem(path);
        item->setData(Qt::UserRole, i);
        mList->addItem(item);
        mVisible.append(i);
    }

    setStatus(tr("%1 of %2 entries shown").arg(mVisible.size()).arg(entryCount()));
}

void ArchiveBrowserDialog::onFilterChanged(int)
{
    rebuildList();
}

void ArchiveBrowserDialog::onSearchTextChanged(const QString&)
{
    rebuildList();
}

void ArchiveBrowserDialog::onEntrySelected(int row)
{
    if (row < 0 || row >= mVisible.size()) return;
    updatePreview(mVisible.at(row));
}

void ArchiveBrowserDialog::onEntryDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    const int index = item->data(Qt::UserRole).toInt();
    if (index < 0) return;
    if (isVisibleByFilter(entryPath(index).toLower(), 3))
        playSelected();
    else
        extractSelected();
}

void ArchiveBrowserDialog::updatePreview(int index)
{
    mSelectedIndex = index;
    const QString path = entryPath(index);
    const QString lower = path.toLower();
    clearPreview();

    const QFileInfo info(path);
    QString infoText = tr("Name: %1\nFolder: %2\nSize: %3 KB")
        .arg(info.fileName())
        .arg(info.path())
        .arg((mKind == Kind::Bsa && mBsa && index < mBsa->entries().size())
                 ? mBsa->entries().at(index).size / 1024 : 0);

    if (isTextureExt(lower))
    {
        QString tmp;
        if (extractToTemp(index, tmp))
        {
            const QImage img = NifViewportWidget::loadTextureImage(tmp);
            QFile::remove(tmp);
            if (!img.isNull())
            {
                mPreviewImage->setPixmap(QPixmap::fromImage(img).scaled(
                    QSize(256, 256), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
            else
            {
                infoText += QStringLiteral("\n(no preview available)");
            }
        }
        mPlayBtn->setEnabled(false);
    }
    else if (lower.endsWith(QStringLiteral(".fuz"))
             || lower.endsWith(QStringLiteral(".wav"))
             || lower.endsWith(QStringLiteral(".xwm"))
             || lower.endsWith(QStringLiteral(".ogg")))
    {
        if (lower.endsWith(QStringLiteral(".wav")))
            infoText += QStringLiteral("\nDouble-click to play.");
        else
            infoText += QStringLiteral("\nDouble-click to play audio.");
        mPlayBtn->setEnabled(true);
    }
    else
    {
        mPlayBtn->setEnabled(false);
    }

    mPreviewInfo->setText(infoText);
}

void ArchiveBrowserDialog::clearPreview()
{
    mPreviewImage->clear();
    if (mPreviewInfo) mPreviewInfo->clear();
}

void ArchiveBrowserDialog::setStatus(const QString& text)
{
    mPreviewInfo->setText(text);
}

void ArchiveBrowserDialog::playSelected()
{
    if (mSelectedIndex < 0) return;
    const int index = mSelectedIndex;
    const QString lower = entryPath(index).toLower();

    if (lower.endsWith(QStringLiteral(".fuz")))
    {
        QByteArray bytes;
        if (!readEntry(index, bytes))
        {
            QMessageBox::warning(this, tr("Play"), tr("Could not read the voice file."));
            return;
        }
        FuzParser fuz;
        if (!FuzParser::parse(bytes, fuz) || !fuz.hasAudio())
        {
            QMessageBox::warning(this, tr("Play"), tr("Unrecognized .fuz container."));
            return;
        }
        if (!VoicePreview::playVoiceAudio(fuz.audioData, fuz.audioFourCC, this))
            QMessageBox::warning(this, tr("Play"), tr("Could not decode the voice audio."));
        return;
    }

    QString tmp;
    if (!extractToTemp(index, tmp))
    {
        QMessageBox::warning(this, tr("Play"), tr("Could not extract the audio file."));
        return;
    }
#ifdef _WIN32
    std::wstring w = tmp.toStdWString();
    if (!PlaySoundW(w.c_str(), nullptr, SND_FILENAME | SND_ASYNC))
    {
        QMessageBox::warning(this, tr("Play"),
            tr("Could not play the audio file (Win32 PlaySound failed)."));
    }
#else
    QMessageBox::information(this, tr("Play"),
        tr("Audio playback is not supported on this platform."));
#endif
}

void ArchiveBrowserDialog::extractSelected()
{
    if (mSelectedIndex < 0) return;

    const QString savePath = QFileDialog::getSaveFileName(this, tr("Extract File"),
        QFileInfo(entryPath(mSelectedIndex)).fileName(), tr("All Files (*.*)"));
    if (savePath.isEmpty()) return;

    if (mKind == Kind::Bsa)
    {
        if (!mBsa->extract(static_cast<quint32>(mSelectedIndex), savePath))
        {
            QMessageBox::critical(this, tr("Extract Failed"),
                tr("Failed to extract: %1").arg(entryPath(mSelectedIndex)));
            return;
        }
    }
    else if (mKind == Kind::Ba2)
    {
        if (!mBa2->extract(static_cast<quint32>(mSelectedIndex), savePath))
        {
            QMessageBox::critical(this, tr("Extract Failed"),
                tr("Failed to extract: %1").arg(entryPath(mSelectedIndex)));
            return;
        }
    }
    else return;

    LOG_INFO(QString("Archive Browser: extracted %1").arg(savePath));
    QMessageBox::information(this, tr("Extracted"),
        tr("File extracted to:\n%1").arg(savePath));
}

void ArchiveBrowserDialog::extractAll()
{
    if (mKind == Kind::None || mVisible.isEmpty()) return;

    const QString dir = QFileDialog::getExistingDirectory(this, tr("Extract Entries To"));
    if (dir.isEmpty()) return;

    int ok = 0;
    int failed = 0;
    for (const int index : mVisible)
    {
        const QString outPath = dir + QStringLiteral("/") + entryPath(index);
        bool success = false;
        if (mKind == Kind::Bsa)
            success = mBsa->extract(static_cast<quint32>(index), outPath);
        else if (mKind == Kind::Ba2)
            success = mBa2->extract(static_cast<quint32>(index), outPath);
        if (success) ++ok;
        else ++failed;
    }

    QMessageBox::information(this, tr("Extraction Complete"),
        tr("Extracted: %1\nFailed: %2").arg(ok).arg(failed));
    LOG_INFO(QString("Archive Browser: extract all - %1 ok, %2 failed").arg(ok).arg(failed));
}
