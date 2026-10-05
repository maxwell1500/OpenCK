#pragma once

#include <QDialog>
#include <QVector>
#include <QByteArray>

class QTreeWidget;
class QTreeWidgetItem;
class QComboBox;
class QLineEdit;
class QPushButton;
class QLabel;
class QListWidgetItem;
class QTreeWidgetItem;
class BsaArchive;
class Ba2Archive;

// Browses Bethesda archive files (BSA and BA2) found in a game data
// directory: filter/search entries, preview textures and sounds (including
// .fuz voices), and extract files to disk.
class ArchiveBrowserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ArchiveBrowserDialog(const QString& dataDirectory = QString(),
                                  QWidget* parent = nullptr);
    ~ArchiveBrowserDialog() override;

    // Writes one entry to an absolute path. quiet suppresses the per-file modal so
    // a batch can report its failures in one summary instead.
    bool extractTo(int index, const QString& outPath, bool quiet = false);
    // Shared by "Extract All Visible" and multi-select extraction: writes each
    // index under `destination`, showing cancellable progress. Returns false if
    // the user cancelled or anything failed.
    bool runBatchExtraction(QVector<int> indices, const QString& destination);
    static bool isSafeExtractionPath(const QString& root, const QString& entryPath,
                                     QString* outputPath = nullptr);

    // Archives under a data root, found recursively and returned sorted.
    // Public so the discovery rule can be tested without a game install: the bug
    // it guards against is a nested layout being silently missed, which the
    // shipped games cannot demonstrate because they keep DLC in the Data root.
    static QStringList findArchives(const QString& root);

private slots:
    void browseArchive();
    void onQuickOpenChanged(int index);
    void onFilterChanged(int index);
    void onSearchTextChanged(const QString& text);
    void onEntrySelected();
    void onEntryDoubleClicked(QTreeWidgetItem* item, int column);
    void playSelected();
    void extractSelected();
    void extractAll();

private:
    void setupUi();
    void openArchive(const QString& path);
    void closeArchive();
    void scanDataDirectory();
    void rebuildList();
    int entryCount() const;
    QString entryPath(int index) const;
    qint64 entrySize(int index) const;
    // -1 where the container does not record the value, which is different from a
    // recorded zero. Callers must not render -1 as a size.
    qint64 entryPackedSize(int index) const;
    qint64 entryOffset(int index) const;
    bool entryCompressed(int index) const;
    // Archive indices behind the current selection, in selection order. Folder
    // rows carry no index and are skipped.
    QVector<int> selectedIndices() const;
    bool readEntry(int index, QByteArray& out) const;
    bool extractToTemp(int index, QString& tmpPath) const;
    void updatePreview(int index);
    void clearPreview();
    void setStatus(const QString& text);
    // Describes what the current filter/search leaves visible, for the extract
    // button's label and its confirmation.
    QString visibleScopeDescription() const;
    static bool isVisibleByFilter(const QString& lowerPath, int filterIndex);
    static bool isTextureExt(const QString& lowerPath);

    enum class Kind { None, Bsa, Ba2 };
    Kind mKind = Kind::None;
    BsaArchive* mBsa = nullptr;
    Ba2Archive* mBa2 = nullptr;
    QVector<int> mVisible;
    int mSelectedIndex = -1;

    QString mDataDirectory;
    QComboBox* mQuickOpen = nullptr;
    QPushButton* mBrowseBtn = nullptr;
    QLabel* mArchiveLabel = nullptr;
    QComboBox* mFilterCombo = nullptr;
    QLineEdit* mSearchEdit = nullptr;
    QTreeWidget* mTree = nullptr;
    QLabel* mPreviewImage = nullptr;
    // Status text and entry metadata used to share one label, so a status message
    // wiped the metadata and vice versa. They are separate widgets now.
    QLabel* mStatusLabel = nullptr;
    QTreeWidget* mMetaTree = nullptr;
    QPushButton* mPlayBtn = nullptr;
    QPushButton* mExtractBtn = nullptr;
    QPushButton* mExtractAllBtn = nullptr;
};
