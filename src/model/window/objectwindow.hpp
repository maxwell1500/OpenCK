#ifndef OBJECTWINDOW_H
#define OBJECTWINDOW_H

#include <QAbstractItemModel>
#include <QStringList>
#include <QVector>
#include <functional>

class Data;
class ObjectWindowFilter;

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
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    int getCategoryIndex(const QModelIndex& index) const;
    int getCategoryType(int categoryId) const;
    int getRecordIndex(const QModelIndex& index) const;
    QModelIndex getCategoryIndexModel(const int categoryId) const;
    QModelIndex getRecordIndexModel(int categoryId, int recordIndex) const;
    const QString& getRecordEditorId(int categoryId, int recordIndex) const;
    const QString& getRecordFormId(int categoryId, int recordIndex) const;

    bool isRecord(const QModelIndex& index) const;

public slots:
    void applyFilter(const QString& text);
    void applyObjectFilter(const ObjectWindowFilter& filter);

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
};

#endif // OBJECTWINDOW_H
