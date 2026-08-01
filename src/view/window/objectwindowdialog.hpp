#ifndef OBJECTWINDOWDIALOG_H
#define OBJECTWINDOWDIALOG_H

#include <QDockWidget>
#include <QTreeView>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>

class ObjectWindowModel;
class Data;
struct CellRecord;

namespace openck { class FormComponents; }

class ObjectWindowDialog : public QDockWidget
{
    Q_OBJECT

public:
    ObjectWindowDialog(Data* data, QWidget* parent = nullptr);
    ~ObjectWindowDialog();
    QModelIndex currentIndex() const;

    struct ClipboardRecord
    {
        int recordType;
        QString editorId;
        quint32 formId;
        QJsonObject fields;
    };

    static bool hasClipboardData();
    static ClipboardRecord getClipboardData();
    static void setClipboardData(const ClipboardRecord& record);
    static void clearClipboardData();

signals:
    void recordSelected(int categoryId, int recordIndex, const QString& editorId);

public slots:
    void filterChanged(const QString& text);
    void editSelected();
    void deleteSelected();
    void cloneSelected();
    void copyRecord();
    void cutRecord();
    void pasteRecord();
    void onDoubleClick(const QModelIndex& index);
    void saveFilter();
    void loadFilter();
    void deleteSavedFilter();

public:
    QTreeView* getTreeView() const { return mTreeView; }
    QLineEdit* getFilterEdit() const { return mFilterEdit; }
    CellRecord* getSelectedCell() const;

    struct RecordLookupResult {
        openck::FormComponents* components = nullptr;
        void* recordPtr = nullptr;
        QString recordType;
    };
    RecordLookupResult getFormComponentsForIndex(int categoryId, int recordIndex) const;

    // Batch Editing (Step 2) - Multi-select support for ObjectWindowDialog
    void enableMultiSelect(bool enabled = true);
    QList<QModelIndex> getSelectedIndices() const;
    
public slots:
    void batchSetEditorId();
    void batchDuplicateIds();
    void openInBlender();
    void previewNif();
    void compareNifs();

private:
    void setupUI();
    void updateContextMenu(const QModelIndex& index);
    int getSelectedCategoryId(const QModelIndex& index) const;
    QString getModelPathForRecord(int categoryId, int recordIndex) const;
    void refreshSavedFilters();

    Data* mData;
    ObjectWindowModel* mModel;
    QTreeView* mTreeView;
    QLineEdit* mFilterEdit;
    QComboBox* mSavedFilterCombo;
    QPushButton* mEditButton;
    QPushButton* mDeleteButton;
    QPushButton* mCloneButton;
    QLabel* mStatusLabel;

    QMenu* mContextMenu;

    static ClipboardRecord sClipboardData;
    static bool sHasClipboardData;
};

#endif // OBJECTWINDOWDIALOG_H
