#ifndef PLUGINMERGEDIALOG_HPP
#define PLUGINMERGEDIALOG_HPP

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QCheckBox>
#include <QTableWidget>
#include <QMap>
#include <QVector>

class Data;

enum class ConflictResolution { AutoRename, KeepSource, KeepDestination };

struct MergeConflict {
    QString recordType;
    QString editorId;
    QString sourceFile;
    ConflictResolution resolution;
};

class PluginMergeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PluginMergeDialog(Data* data, QWidget* parent = nullptr);
    ~PluginMergeDialog();

private slots:
    void onMerge();
    void onSelectAll();
    void onDeselectAll();
    void onGeneratePreview();

private:
    void setupUI();
    QCheckBox* makeCheckbox(const QString& text, bool checked);
    bool mergeType(Data* data, const QString& typeName, const QString& sourceFile,
                   const QMap<QString, ConflictResolution>& resolutions,
                   QVector<MergeConflict>& newConflicts);
    bool showManualResolutionDialog();
    void collectConflicts();

    Data* mData;
    QListWidget* sourceList;
    QListWidget* destList;
    QPushButton* mergeButton;
    QPushButton* selectAllButton;
    QPushButton* deselectAllButton;
    QPushButton* previewButton;
    QProgressBar* progressBar;
    QLabel* statusLabel;
    QWidget* recordTypeCheckboxes;
    QTableWidget* mPreviewTable;
    QLabel* mPreviewStats;

    QVector<MergeConflict> mConflicts;
    QStringList mSourceFiles;
    QStringList mSelectedTypes;
};

#endif // PLUGINMERGEDIALOG_HPP
