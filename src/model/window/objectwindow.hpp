#ifndef OBJECTWINDOW_H
#define OBJECTWINDOW_H

#include <QAbstractItemModel>
#include <QStringList>
#include <QVector>
#include <QTimer>
#include <functional>

#include "../tools/objectwindowfilter.hpp"
#include "../world/ckid.hpp"

class Data;

/// A folder in a category's model-directory tree (MODL paths minus the
/// Data\Meshes prefix). `path` is lowercase, '/'-separated; `count` includes
/// every record in the folder's subtree.
struct ModelFolder
{
    QString path;
    QString name;
    int count = 0;
};

/// Tree model presenting records grouped by category for the Object Window.
class ObjectWindowModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ObjectWindowModel(QObject* parent = nullptr);

    void setData(Data* data);

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    // Deferred-master categories (indexed but not yet parsed) report their
    // records through canFetchMore/fetchMore: expanding the category
    // materializes the record type from the master index.
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

    int getCategoryIndex(const QModelIndex& index) const;
    int getCategoryType(int categoryId) const;
    int getRecordIndex(const QModelIndex& index) const;
    QModelIndex getCategoryIndexModel(const int categoryId) const;
    QModelIndex getRecordIndexModel(int categoryId, int recordIndex) const;
    const QString& getRecordEditorId(int categoryId, int recordIndex) const;
    const QString& getRecordFormId(int categoryId, int recordIndex) const;
    QString getRecordModelPath(int categoryId, int recordIndex) const;
    int getCategoryCount() const { return mCategories.size(); }
    int getRecordCountForCategory(int categoryId) const;
    int getRecordActualIndex(int categoryId, int visibleIndex) const;
    const QString& getVisibleRecordEditorId(int categoryId, int visibleIndex) const;
    const QString& getVisibleRecordFormId(int categoryId, int visibleIndex) const;
    QString getVisibleRecordModelPath(int categoryId, int visibleIndex) const;
    QString getCategoryName(int categoryId) const;
    /// Lowercase '/'-separated directory of a MODL path, without the
    /// Data\Meshes prefix; empty when the path has no directory.
    static QString modelFolder(const QString& modelPath);
    /// Pseudo-folder path for records whose model path has no directory
    /// (or no model at all); only listed when the category has real folders.
    static inline const QString NoFolder = QStringLiteral("/");
    /// Folder tree of the category's visible records, parents sorted before children.
    QVector<ModelFolder> getModelFolders(int categoryId) const;

    bool findRecord(const QString& idOrFormId, int& outCategoryId, int& outRecordIndex, QModelIndex& outIndex) const;
    bool isRecord(const QModelIndex& index) const;

public slots:
    void applyFilter(const QString& text);
    void applyObjectFilter(const ObjectWindowFilter& filter);

    // Returns the most recently applied structured filter (may be empty).
    ObjectWindowFilter activeObjectFilter() const { return mActiveObjectFilter; }

private slots:
    void materializeTick();

private:
    struct VisibleRecord
    {
        int actualIndex;
        QString editorId;
        QString formId;
    };

    /// A record category (e.g. "Weapons", "Armor") holding its visible records.
    struct Category
    {
        QString name;
        int typeId;
        int totalRecords;
        int parsedCount = 0;
        bool pendingMaterialize = false;
        QVector<VisibleRecord> visibleRecords;
    };

    /// A top-level group of related categories in the Object Window tree.
    struct CategoryGroup
    {
        QString name;
        QVector<int> categoryIndices;
    };

    void initCategories(Data* data);
    QString formatFormId(quint32 formId) const;
    void rebuildAllRecords();
    void appendMaterializedRows(int groupRow, int categoryRow, int firstNew, const QModelIndex& parent);
    void startMaterializeTimer();

    bool isGroupNode(const QModelIndex& index) const;
    bool isCategoryNode(const QModelIndex& index) const;
    bool isRecordNode(const QModelIndex& index) const;
    int flatCategoryId(int groupRow, int categoryRow) const;
    bool findCategoryLocation(int flatId, int& groupRow, int& categoryRow) const;

    static constexpr quintptr kGroupInternalId = 0;
    static constexpr quintptr kRecordBit = 0x80000000u;

    Data* mData;
    QVector<Category> mCategories;
    QVector<CategoryGroup> mGroups;
    QString mFilter;
    ObjectWindowFilter mActiveObjectFilter;

    // Time-sliced deferred-master materialization driven by a QTimer so the
    // UI thread never blocks inside fetchMore(). Row insertion still happens
    // with beginInsertRows/endInsertRows on the UI thread (never a model
    // reset while QTreeView is expanding).
    QTimer mMaterializeTimer;
    struct MaterializeJob
    {
        int typeId = CkId::Type_None;
        int groupRow = -1;
        int categoryRow = -1;
        int flatId = -1;
        int firstNew = 0;
        bool active = false;
    };
    MaterializeJob mJob;
};

#endif // OBJECTWINDOW_H
