#pragma once

#include <QDialog>
#include <QVector>
#include <QByteArray>

class QListWidget;
class QComboBox;
class QLineEdit;
class QPushButton;
class QLabel;
class QListWidgetItem;
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

private slots:
    void browseArchive();
    void onQuickOpenChanged(int index);
    void onFilterChanged(int index);
    void onSearchTextChanged(const QString& text);
    void onEntrySelected(int index);
    void onEntryDoubleClicked(QListWidgetItem* item);
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
    bool readEntry(int index, QByteArray& out) const;
    bool extractToTemp(int index, QString& tmpPath) const;
    void updatePreview(int index);
    void clearPreview();
    void setStatus(const QString& text);
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
    QListWidget* mList = nullptr;
    QLabel* mPreviewImage = nullptr;
    QLabel* mPreviewInfo = nullptr;
    QPushButton* mPlayBtn = nullptr;
    QPushButton* mExtractBtn = nullptr;
    QPushButton* mExtractAllBtn = nullptr;
};
