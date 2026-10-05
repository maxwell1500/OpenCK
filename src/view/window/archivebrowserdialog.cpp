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
#include <QHash>
#include <QHBoxLayout>
#include <QImage>
#include <QLineEdit>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <mmsystem.h>
#  pragma comment(lib, "winmm.lib")
#endif

namespace {

bool safeArchiveOutputPath(const QString& root, const QString& entryPath, QString& outputPath)
{
    const QString normalized = QDir::fromNativeSeparators(entryPath).trimmed();
    if (normalized.isEmpty() || normalized.startsWith('/') || normalized.startsWith('\\')
        || normalized.contains(':') || QDir::isAbsolutePath(normalized))
        return false;

    const QStringList parts = normalized.split('/', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return false;
    for (const QString& part : parts)
        if (part == QStringLiteral("..")) return false;

    const QString cleanRelative = QDir::cleanPath(normalized).replace('\\', '/');
    if (cleanRelative.isEmpty() || cleanRelative == QStringLiteral(".")
        || cleanRelative == QStringLiteral("..")
        || cleanRelative.startsWith(QStringLiteral("../")))
        return false;

    const QString cleanRoot = QDir::cleanPath(QDir(root).absolutePath()).replace('\\', '/');
    const QString candidate = QDir::cleanPath(cleanRoot + '/' + cleanRelative).replace('\\', '/');
    if (candidate != cleanRoot
        && !candidate.startsWith(cleanRoot + '/', Qt::CaseInsensitive))
        return false;
    outputPath = candidate;
    return true;
}

}

bool ArchiveBrowserDialog::isSafeExtractionPath(const QString& root, const QString& entryPath,
                                                QString* outputPath)
{
    QString result;
    if (!safeArchiveOutputPath(root, entryPath, result)) return false;
    if (outputPath) *outputPath = result;
    return true;
}

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
    // "All" is worded as "Everything" rather than "All Files" because the filter
    // is about entry types, not about which files exist. The distinction matters
    // to the extract button below, which acts on whatever this leaves visible.
    mFilterCombo->addItems({
        tr("Everything"), tr("Models"), tr("Textures"), tr("Sounds"), tr("Voice (.fuz)")
    });
    mSearchEdit = new QLineEdit(this);
    mSearchEdit->setObjectName(QStringLiteral("searchEdit"));
    mSearchEdit->setPlaceholderText(tr("Search entries..."));
    filterRow->addWidget(mFilterCombo);
    filterRow->addWidget(mSearchEdit, 1);
    topLayout->addLayout(filterRow);

    // A tree rather than a flat list: a mesh archive holds tens of thousands of
    // entries laid out in folders, and a flat list of full paths cannot be
    // scanned or navigated at that size. Columns carry the uncompressed size so a
    // file can be judged before extracting it, and sorting is per-column.
    mTree = new QTreeWidget(this);
    mTree->setObjectName(QStringLiteral("entryTree"));
    mTree->setColumnCount(2);
    mTree->setHeaderLabels({ tr("Name"), tr("Size") });
    mTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mTree->setUniformRowHeights(true);
    mTree->setSortingEnabled(true);
    mTree->setExpandsOnDoubleClick(false);
    topLayout->addWidget(mTree, 1);

    auto* btnRow = new QHBoxLayout();
    mPlayBtn = new QPushButton(tr("Play"), this);
    mExtractBtn = new QPushButton(tr("Extract Selected..."), this);
    mExtractAllBtn = new QPushButton(tr("Extract Visible..."), this);
    mExtractAllBtn->setObjectName(QStringLiteral("extractAllBtn"));
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
    connect(mTree, &QTreeWidget::itemSelectionChanged,
            this, &ArchiveBrowserDialog::onEntrySelected);
    connect(mTree, &QTreeWidget::itemDoubleClicked,
            this, &ArchiveBrowserDialog::onEntryDoubleClicked);
    connect(mPlayBtn, &QPushButton::clicked, this, &ArchiveBrowserDialog::playSelected);
    connect(mExtractBtn, &QPushButton::clicked, this, &ArchiveBrowserDialog::extractSelected);
    connect(mExtractAllBtn, &QPushButton::clicked, this, &ArchiveBrowserDialog::extractAll);
}

// Archives are looked for recursively. Fallout 4 and Skyrim keep their DLC in
// the Data root, so a flat listing happened to work for them, but any install
// that nests its content — and the CK's own resource-archive list is not
// root-only either — would silently be missing from the quick-open list.
// Written as an explicit descent rather than handed to QDirIterator or to
// entryList's recursion flag: both hide the traversal rule behind a default, and
// this is the one place where "did we recurse" is the entire behaviour under
// test. Matching is by suffix rather than by QDir name patterns, because a name
// is matched against *every* pattern unless disjunction is requested — so
// "*.bsa" and "*.ba2" together silently match neither.
namespace {

bool isArchiveSuffix(const QString& suffix)
{
    return suffix.compare(QLatin1String("bsa"), Qt::CaseInsensitive) == 0
        || suffix.compare(QLatin1String("ba2"), Qt::CaseInsensitive) == 0;
}

void collectArchives(const QDir& dir, QStringList& out)
{
    const auto entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                                           QDir::Name);
    for (const QFileInfo& info : entries) {
        if (info.isDir()) {
            collectArchives(QDir(info.absoluteFilePath()), out);
        } else if (isArchiveSuffix(info.suffix())) {
            out << info.absoluteFilePath();
        }
    }
}

} // namespace

QStringList ArchiveBrowserDialog::findArchives(const QString& root)
{
    QStringList found;
    if (root.isEmpty()) return found;
    const QDir dataRoot(root);
    if (!dataRoot.exists()) return found;
    collectArchives(dataRoot, found);
    found.sort();
    return found;
}

void ArchiveBrowserDialog::scanDataDirectory()
{
    if (mDataDirectory.isEmpty()) return;

    const QStringList archivePaths = findArchives(mDataDirectory);

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

qint64 ArchiveBrowserDialog::entrySize(int index) const
{
    // Uncompressed size on both sides: compressed size would misreport the
    // archive and mislead anyone deciding whether to extract.
    switch (mKind)
    {
    case Kind::Bsa:
        if (mBsa && index >= 0 && index < mBsa->entries().size())
            return static_cast<qint64>(mBsa->entries().at(index).size);
        break;
    case Kind::Ba2:
        if (mBa2 && index >= 0 && index < static_cast<int>(mBa2->entries().size()))
            return static_cast<qint64>(mBa2->entries().at(index).uncompressedSize);
        break;
    default: break;
    }
    return 0;
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
    mTree->clear();
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

    mVisible.reserve(entryCount());
    // QTreeWidget, unlike QListWidget, does not adopt an item constructed with a
    // null parent: it stays detached and invisible until addTopLevelItem() claims
    // it. Every row therefore goes through one of these two helpers.
    const auto addRow = [this](QTreeWidgetItem* parent) {
        if (parent) return new QTreeWidgetItem(parent);
        auto* item = new QTreeWidgetItem();
        mTree->addTopLevelItem(item);
        return item;
    };
    // Folder nodes are created on demand and reused, so an archive with many
    // files per folder builds one row per folder rather than one per file.
    QHash<QString, QTreeWidgetItem*> folders;
    const auto folderFor = [&folders, &addRow](const QStringList& segments, int upto) {
        QTreeWidgetItem* parent = nullptr;
        QString key;
        for (int s = 0; s < upto; ++s) {
            key += QLatin1Char('/') + segments.at(s).toLower();
            QTreeWidgetItem*& slot = folders[key];
            if (!slot)
                slot = addRow(parent);
            // No reparenting is needed: a folder key is a full path from the
            // archive root, so its parent is the same every time it is reached,
            // and the walk above has already created it.
            slot->setText(0, segments.at(s));
            parent = slot;
        }
        return parent;
    };

    mTree->setUpdatesEnabled(false);
    for (int i = 0; i < entryCount(); ++i)
    {
        const QString path = entryPath(i);
        const QString lower = path.toLower();
        if (!isVisibleByFilter(lower, filterIndex)) continue;
        if (!search.isEmpty() && !lower.contains(search)) continue;

        // BSA stores entry names as "folder\file" while BA2 uses forward
        // slashes, so both separators have to be honoured or a BSA's folders
        // never split and the whole archive appears as one flat folder row.
        QStringList segments = path.split(QRegularExpression(QStringLiteral("[/\\\\]")),
                                          Qt::SkipEmptyParts);
        if (segments.isEmpty()) continue;
        QTreeWidgetItem* parent = folderFor(segments, segments.size() - 1);
        auto* item = addRow(parent);
        item->setText(0, segments.last());
        item->setText(1, QLocale().formattedDataSize(entrySize(i)));
        item->setData(0, Qt::UserRole, i);
        // A stable secondary sort key, so numeric ordering does not depend on the
        // formatted string ("1 KB" sorting before "900 KB").
        item->setData(1, Qt::UserRole, static_cast<qlonglong>(entrySize(i)));
        mVisible.append(i);
    }
    mTree->setUpdatesEnabled(true);
    mTree->expandToDepth(0);
    mTree->resizeColumnToContents(0);

    setStatus(tr("%1 of %2 entries shown in %3 folder(s)")
                  .arg(mVisible.size()).arg(entryCount()).arg(mTree->topLevelItemCount()));

    // Keep the button honest about its scope as the filter and search change.
    // When nothing is filtered it can offer the whole archive; once a filter or
    // search narrows the list, the label has to say so.
    if (mKind == Kind::None || mVisible.isEmpty()) {
        mExtractAllBtn->setEnabled(false);
        mExtractAllBtn->setText(tr("Extract Visible..."));
    } else if (mVisible.size() == entryCount()) {
        mExtractAllBtn->setEnabled(true);
        mExtractAllBtn->setText(tr("Extract All %1...").arg(entryCount()));
    } else {
        mExtractAllBtn->setEnabled(true);
        mExtractAllBtn->setText(tr("Extract %1 Visible...").arg(mVisible.size()));
    }
}

void ArchiveBrowserDialog::onFilterChanged(int)
{
    rebuildList();
}

void ArchiveBrowserDialog::onSearchTextChanged(const QString&)
{
    rebuildList();
}

void ArchiveBrowserDialog::onEntrySelected()
{
    const QVector<int> selected = selectedIndices();
    mSelectedIndex = selected.isEmpty() ? -1 : selected.first();
    // Preview and the action buttons follow the first selected file; a folder row
    // has no archive index and so leaves both disabled.
    mExtractBtn->setEnabled(mSelectedIndex >= 0);
    mPlayBtn->setEnabled(mSelectedIndex >= 0
        && isVisibleByFilter(entryPath(mSelectedIndex).toLower(), 3));
    if (mSelectedIndex < 0) {
        clearPreview();
        return;
    }
    updatePreview(mSelectedIndex);
}

void ArchiveBrowserDialog::onEntryDoubleClicked(QTreeWidgetItem* item, int)
{
    if (!item) return;
    const QVariant stored = item->data(0, Qt::UserRole);
    // Folder rows have no archive index; expanding is handled by setExpandsOnDoubleClick.
    if (!stored.isValid()) return;
    const int index = stored.toInt();
    if (index < 0) return;
    mSelectedIndex = index;
    if (isVisibleByFilter(entryPath(index).toLower(), 3))
        playSelected();
    else
        extractSelected();
}

QVector<int> ArchiveBrowserDialog::selectedIndices() const
{
    QVector<int> out;
    const QList<QTreeWidgetItem*> items = mTree->selectedItems();
    out.reserve(items.size());
    for (QTreeWidgetItem* item : items) {
        const QVariant stored = item->data(0, Qt::UserRole);
        if (stored.isValid())
            out.append(stored.toInt());
    }
    return out;
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
    const QVector<int> selected = selectedIndices();
    if (selected.isEmpty()) return;

    // More than one file cannot sensibly be written to one chosen path, so a
    // multi-selection is extracted into a chosen directory, preserving the
    // archive's folder layout under it.
    if (selected.size() > 1) {
        const QString dir = QFileDialog::getExistingDirectory(
            this, tr("Extract %1 Files Into").arg(selected.size()));
        if (dir.isEmpty()) return;

        int failed = 0;
        QString firstError;
        for (int index : selected) {
            const QString rel = entryPath(index);
            QString outPath;
            // Same containment check and directory creation as extractAll(); an
            // archive entry name is not trusted to be a safe relative path.
            if (!isSafeExtractionPath(dir, rel, &outPath)
                || !QDir().mkpath(QFileInfo(outPath).absolutePath())
                || !extractTo(index, outPath, /*quiet=*/true)) {
                ++failed;
                if (firstError.isEmpty()) firstError = rel;
            }
        }
        LOG_INFO(QString("Archive Browser: extracted %1 selected file(s) to %2 (%3 failed)")
                     .arg(selected.size()).arg(dir).arg(failed));
        if (failed > 0) {
            QMessageBox::warning(this, tr("Extraction Incomplete"),
                tr("%n file(s) could not be written, starting with:\n%1\n\n"
                   "Nothing outside the chosen folder was written.", nullptr, failed)
                    .arg(firstError));
        } else {
            QMessageBox::information(this, tr("Extracted"),
                tr("%n file(s) extracted to:\n%1", nullptr, selected.size()).arg(dir));
        }
        return;
    }

    const int index = selected.first();
    const QString savePath = QFileDialog::getSaveFileName(this, tr("Extract File"),
        QFileInfo(entryPath(index)).fileName(), tr("All Files (*.*)"));
    if (savePath.isEmpty()) return;

    if (!extractTo(index, savePath, /*quiet=*/false)) return;

    LOG_INFO(QString("Archive Browser: extracted %1").arg(savePath));
    QMessageBox::information(this, tr("Extracted"),
        tr("File extracted to:\n%1").arg(savePath));
}

bool ArchiveBrowserDialog::extractTo(int index, const QString& outPath, bool quiet)
{
    bool ok = false;
    if (mKind == Kind::Bsa && mBsa)
        ok = mBsa->extract(static_cast<quint32>(index), outPath);
    else if (mKind == Kind::Ba2 && mBa2)
        ok = mBa2->extract(static_cast<quint32>(index), outPath);
    // In a batch, one failure is reported in the summary rather than by popping a
    // modal per file, which would make extracting hundreds unwieldy.
    if (!ok && !quiet)
        QMessageBox::critical(this, tr("Extract Failed"),
            tr("Failed to extract: %1").arg(entryPath(index)));
    return ok;
}

// A user-facing description of what the filter and search box currently leave
// visible, used by the extract button's label and its confirmation. An archive is
// far too large to extract wholesale by accident, so this always names the scope
// rather than leaning on the button text alone.
QString ArchiveBrowserDialog::visibleScopeDescription() const
{
    const int total = entryCount();
    const int visible = mVisible.size();
    const QString filter = mFilterCombo ? mFilterCombo->currentText() : QString();
    if (visible == total)
        return tr("every entry (%1)").arg(total);
    return tr("the %1 entries matching \"%2\" (%3 of %4)")
        .arg(visible)
        .arg(filter.isEmpty() ? tr("All") : filter)
        .arg(visible)
        .arg(total);
}

void ArchiveBrowserDialog::extractAll()
{
    if (mKind == Kind::None || mVisible.isEmpty()) return;

    // The button used to read "Extract All..." while iterating the *filtered*
    // set, so a user who narrowed the list to a few dozen textures and clicked it
    // had no way to tell that the archive's other 30,000-odd entries were not
    // also going to be written. State the scope and the count before asking for
    // a destination, so the decision is informed.
    const QString scope = visibleScopeDescription();
    const QString prompt = tr("Extract %1 to a folder?\n\n%2 will be written.")
                               .arg(scope, tr("%1 entries match the current filter.")
                                       .arg(mVisible.size()));
    if (QMessageBox::question(this, tr("Extract Entries"), prompt,
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    const QString dir = QFileDialog::getExistingDirectory(this, tr("Extract Entries To"));
    if (dir.isEmpty()) return;

    int ok = 0;
    int failed = 0;
    for (const int index : mVisible)
    {
        QString outPath;
        if (!isSafeExtractionPath(dir, entryPath(index), &outPath))
        {
            ++failed;
            LOG_WARNING(QString("Archive Browser: rejected unsafe extraction path: %1")
                .arg(entryPath(index)));
            continue;
        }
        if (!QDir().mkpath(QFileInfo(outPath).absolutePath()))
        {
            ++failed;
            continue;
        }
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
