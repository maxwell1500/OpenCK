#ifndef OBJECTWINDOW_H
#define OBJECTWINDOW_H

#include <QAbstractItemModel>
#include <QStringList>
#include <QVector>
#include <functional>

class Data;

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

public slots:
    void applyFilter(const QString& text);

private:
    struct VisibleRecord
    {
        int actualIndex;
        QString editorId;
        QString formId;
    };

    struct Category
    {
        QString name;
        int typeId;
        int totalRecords;
        QVector<VisibleRecord> visibleRecords;
    };

    void initCategories(Data* data);
    QString formatFormId(quint32 formId) const;

    Data* mData;
    QVector<Category> mCategories;
    QString mFilter;
};

#endif // OBJECTWINDOW_H
