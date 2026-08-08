#ifndef COLLECTION_H
#define COLLECTION_H

#include "basecollection.hpp"
#include "basecolumn.hpp"
#include "record.hpp"

#include <algorithm>
#include <type_traits>

#include <QMap>
#include <QVector>

template<typename T, typename = void>
struct HasFormIdField : std::false_type {};

template<typename T>
struct HasFormIdField<T, std::void_t<decltype(std::declval<T>().formId)>> : std::true_type {};

const int NOT_FOUND = -1;

template<typename ESXRecord>
struct IdAccessor
{
    void setId(ESXRecord& record, const QString& id) const;
    QString getId(const ESXRecord& record) const;
};

template<typename ESXRecord>
void IdAccessor<ESXRecord>::setId(ESXRecord& record, const QString& id) const
{
    // Try editorId first (most record types use this), fall back to id
    record.editorId = id;
}

template<typename ESXRecord>
QString IdAccessor<ESXRecord>::getId(const ESXRecord& record) const
{
    // Try editorId first (most record types use this), fall back to id
    return record.editorId;
}

template<typename ESXRecord, typename IdAccessorT = IdAccessor<ESXRecord>>
class Collection : public BaseCollection
{
private:
    QVector<Record<ESXRecord>> records;
    QMap<QString, int> indexes;
    QVector<Column<ESXRecord>*> columns;
    mutable QVector<quint32> mDeletedFormIds;

    Collection(const Collection&) = delete;
    Collection& operator=(const Collection&) = delete;

protected:
    const QMap<QString, int>& getIdMap() const;

public:
    const QVector<Record<ESXRecord>>& getRecords() const;

    bool reorderRowsImp(int baseIndex, const QVector<int>& newOrder);
    int cloneRecordImp(const QString& src, const QString& dest, CkId::Type type);
    int touchRecordImp(const QString& id);

public:
    using value_type = ESXRecord;

    Collection();
    virtual ~Collection();

    void add(const ESXRecord& record);

    virtual int size() const;
    virtual QString getId(const int index) const;
    virtual int getIndex(const QString& id) const;
    virtual int getColumns() const;
    virtual QVariant getData(int index, int column) const;
    virtual void setData(int index, int column, const QVariant& data);
    virtual const BaseColumn& getColumn(int column) const;
    virtual void merge();
    virtual void purge();
    virtual void removeRows(int index, int count);
    virtual void appendBlankRecord(const QString& id, CkId::Type = CkId::Type_None);
    virtual void cloneRecord(const QString& src, const QString& dest, const CkId::Type type);
    virtual bool touchRecord(const QString& id);
    virtual int searchId(const QString& id) const;
    virtual QVector<QString> getIds(bool listDeleted = true) const;
    virtual void replace(const int index, const BaseRecord& record);
    virtual void insertRecord(const BaseRecord& record, int index, CkId::Type = CkId::Type_None);
    virtual void appendRecord(const BaseRecord& record, CkId::Type = CkId::Type_None);
    virtual const Record<ESXRecord>& getRecord(const int index) const;
    virtual const Record<ESXRecord>& getRecord(const QString& id) const;
    Record<ESXRecord>& getRecord(const int index);
    Record<ESXRecord>& getRecord(const QString& id);
    virtual int getAppendIndex(const QString& id, CkId::Type = CkId::Type_None) const;
    virtual bool reorderRows(int baseIndex, const QVector<int>& newOrder);
    
    void addColumn(Column<ESXRecord>* column);
    void setRecord(int index, const Record<ESXRecord>& record);
    void renameRecord(int index, const QString& newEditorId);

    NestableColumn* getNestableColumn(int column) const;

    virtual std::unique_ptr<BaseRecord> cloneRecordAt(int index) const override;

    // IRecordCollection overrides
    quint32 getFormId(int index) const override;
    void setFormId(int index, quint32 formId) override;
    bool containsFormId(quint32 formId) const override;
    bool isRecordModified(int index) const override;
    void saveModifiedRecords(ESMWriter& writer, uint32_t recordType) const override;

    // Undo-aware operations
    bool removeRecordWithUndo(const QString& id, UndoStack* undoStack) override;
    bool cloneRecordWithUndo(const QString& src, const QString& dest, UndoStack* undoStack) override;
    void batchCloneWithUndo(const QVector<QString>& srcIds, const QVector<QString>& destIds, UndoStack* undoStack) override;
    void batchSetEditorIdWithUndo(const QVector<QString>& srcIds, const QString& newEditorId, UndoStack* undoStack) override;
};

template<typename ESXRecord, typename IdAccessorT>
Collection<ESXRecord, IdAccessorT>::Collection()
{
}

template<typename ESXRecord, typename IdAccessorT>
const QMap<QString, int>& Collection<ESXRecord, IdAccessorT>::getIdMap() const
{
    return indexes;
}

template<typename ESXRecord, typename IdAccessorT>
const QVector<Record<ESXRecord>>& Collection<ESXRecord, IdAccessorT>::getRecords() const
{
    return records;
}

template<typename ESXRecord, typename IdAccessorT>
bool Collection<ESXRecord, IdAccessorT>::reorderRowsImp(int baseIndex, const QVector<int>& newOrder)
{
    if (!newOrder.isEmpty())
    {
        int size = newOrder.size();

        QVector<int> test(newOrder);
        std::sort(test.begin(), test.end());

        if (test.first() != baseIndex || test.last() != baseIndex + size - 1)
        {
            return false;
        }

        QVector<Record<ESXRecord>> temp(size);
        QMap<int, int> positionMapping;

        for (int i = 0; i < size; i++)
        {
            int newPos = newOrder[i] - baseIndex;
            temp[newPos] = records[baseIndex + i];
            temp[newPos].setModified(temp[newPos].get());
            positionMapping[baseIndex + i] = newOrder[i];
        }

        std::copy(temp.begin(), temp.end(), records.begin() + baseIndex);

        for (auto it = indexes.begin(); it != indexes.end(); ++it)
        {
            if (it.value() >= baseIndex && it.value() < baseIndex + size)
            {
                it.value() = positionMapping[it.value()];
            }
        }
    }

    return true;
}

template<typename ESXRecord, typename IdAccessorT>
int Collection<ESXRecord, IdAccessorT>::cloneRecordImp(const QString& src, const QString& dest, CkId::Type type)
{
    Record<ESXRecord> copy;
    copy.setModified(getRecord(src).get());
    copy.state = State::State_ModifiedOnly;

    IdAccessorT().setId(copy.get(), dest);

    int index = getAppendIndex(dest, type);
    insertRecord(copy, index);

    return index;
}

template<typename ESXRecord, typename IdAccessorT>
int Collection<ESXRecord, IdAccessorT>::touchRecordImp(const QString& id)
{
    int index = getIndex(id);
    Record<ESXRecord>& record = records[index];

    if (record.isDeleted())
    {
        throw std::runtime_error("Attempted to touch a deleted record");
    }

    if (!record.isModified())
    {
        record.setModified(record.get());
        return index;
    }

    return -1;
}

template<typename ESXRecord, typename IdAccessorT>
void Collection<ESXRecord, IdAccessorT>::cloneRecord(const QString& src, const QString& dest, const CkId::Type type)
{
    cloneRecordImp(src, dest, type);
}

template<typename ESXRecord, typename IdAccessorT>
bool Collection<ESXRecord, IdAccessorT>::touchRecord(const QString& id)
{
    return touchRecordImp(id) != -1;
}

template<typename ESXRecord, typename IdAccessorT>
Collection<ESXRecord, IdAccessorT>::~Collection()
{
    for (auto it = columns.begin(); it != columns.end(); ++it)
    {
        delete* it;
    }
}

template<typename ESXRecord, typename IdAccessorT>
void Collection<ESXRecord, IdAccessorT>::add(const ESXRecord& record)
{
    QString id = IdAccessorT().getId(record).toLower();

    auto it = indexes.find(id);

    if (it == indexes.end())
    {
        Record<ESXRecord> newRecord;
        newRecord.state = State::State_ModifiedOnly;
        newRecord.modifiedRecord = record;

        insertRecord(newRecord, getAppendIndex(id));
    }
    else
    {
        records[it.value()].setModified(record);
    }
}

template<typename ESXRecord, typename IdAccessorT>
int Collection<ESXRecord, IdAccessorT>::getColumns() const
{
    return columns.size();
}

template<typename ESXRecord, typename IdAccessorT>
QVariant Collection<ESXRecord, IdAccessorT>::getData(int index, int column) const
{
    return columns.at(column)->get(records.at(index));
}

template<typename ESXRecord, typename IdAccessorT>
void Collection<ESXRecord, IdAccessorT>::setData(int index, int column, const QVariant& data)
{
    columns.at(column)->set(records[index], data);
}

template<typename ESXRecord, typename IdAccessorT>
const BaseColumn& Collection<ESXRecord, IdAccessorT>::getColumn(int column) const
{
    return *columns.at(column);
}

template<typename ESXRecord, typename IdAccessorT>
NestableColumn* Collection<ESXRecord, IdAccessorT>::getNestableColumn(int column) const
{
    if (column < 0 || column >= columns.size())
    {
        throw std::runtime_error("Column index out of range");
    }

    return columns[column];
}

template<typename ESXRecord, typename IdAccessorT>
void Collection<ESXRecord, IdAccessorT>::addColumn(Column<ESXRecord>* column)
{
    columns.push_back(column);
}

template<typename ESXRecord, typename IdAccessorT>
void Collection<ESXRecord, IdAccessorT>::merge()
{
    for (auto it = records.begin(); it != records.end(); ++it)
    {
        it->merge();
    }

    purge();
}

template<typename ESXRecord, typename IdAccessorT>
void Collection<ESXRecord, IdAccessorT>::purge()
{
    int i = 0;
    
    while (i < records.size())
    {
        if (records[i].isErased())
        {
            removeRows(i, 1);
        }
        else
        {
            i++;
        }
    }
    mDeletedFormIds.clear();
}

template<typename ESXRecord, typename IdAccessorT>
void Collection<ESXRecord, IdAccessorT>::removeRows(int index, int count)
{
    records.erase(records.begin() + index, records.begin() + index + count);
    auto it = indexes.begin();

    while (it != indexes.end())
    {
        if (it.value() >= index)
        {
            if (it.value() >= index + count)
            {
                it.value() -= count;
                it++;
            }
            else
            {
                indexes.erase(it++);
            }
        }
        else
        {
            ++it;
        }
    }
}

template<typename ESXRecord, typename IdAccessorT>
void Collection<ESXRecord, IdAccessorT>::appendBlankRecord(const QString& id, CkId::Type type)
{
    ESXRecord record;
    IdAccessorT().setId(record, id);

    Record<ESXRecord> newRecord;
    newRecord.state = State::State_ModifiedOnly;
    newRecord.modifiedRecord = record;

    insertRecord(newRecord, getAppendIndex(id, type), type);
}

template<typename ESXRecord, typename IdAccessorT>
int Collection<ESXRecord, IdAccessorT>::getIndex(const QString& id) const
{
    int index = searchId(id);

    if (index == NOT_FOUND)
    {
        throw std::runtime_error("Invalid id: " + id.toStdString());
    }
    else
    {
        return index;
    }
}

template<typename ESXRecord, typename IdAccessorT>
int Collection<ESXRecord, IdAccessorT>::searchId(const QString& id) const
{
    QString lowerId = id.toLower();
    QMap<QString, int>::const_iterator it = indexes.find(lowerId);

    if (it == indexes.end())
    {
        return NOT_FOUND;
    }
    else
    {
        return it.value();
    }
}

template<typename ESXRecord, typename IdAccessorT>
QVector<QString> Collection<ESXRecord, IdAccessorT>::getIds(bool listDeleted) const
{
    QVector<QString> ids;

    for (QMap<QString, int>::const_iterator it = indexes.begin(); it != indexes.end(); ++it)
    {
        if (listDeleted || !records.at(it.value()).isDeleted())
        {
            ids.push_back(IdAccessorT().getId(records.at(it.value()).get()));
        }
    }

    return ids;
}

template<typename ESXRecord, typename IdAccessorT>
void Collection<ESXRecord, IdAccessorT>::replace(int index, const BaseRecord& record)
{
    records.replace(index, dynamic_cast<const Record<ESXRecord>&>(record));
}

template<typename ESXRecord, typename IdAccessorT>
void Collection<ESXRecord, IdAccessorT>::insertRecord(const BaseRecord& record, int index, CkId::Type type)
{
    const Record<ESXRecord>& rec = dynamic_cast<const Record<ESXRecord>&>(record);
    records.insert(records.begin() + index, rec);

    if (index < static_cast<int>(records.size() - 1))
    {
        for (QMap<QString, int>::iterator it = indexes.begin(); it != indexes.end(); ++it)
        {
            if (it.value() >= index)
            {
                ++(it.value());
            }
        }
    }

    indexes.insert(IdAccessorT().getId(rec.get()), index);
}

template<typename ESXRecord, typename IdAccessorT>
void Collection<ESXRecord, IdAccessorT>::appendRecord(const BaseRecord& record, CkId::Type type)
{
    insertRecord(record, getAppendIndex(IdAccessorT().getId(dynamic_cast<const Record<ESXRecord>&>(record).get())), type);
}

template<typename ESXRecord, typename IdAccessorT>
int Collection<ESXRecord, IdAccessorT>::size() const
{
    return records.size();
}

template<typename ESXRecord, typename IdAccessorT>
QString Collection<ESXRecord, IdAccessorT>::getId(const int index) const
{
    return IdAccessorT().getId(records[index].get());
}

template<typename ESXRecord, typename IdAccessorT>
const Record<ESXRecord>& Collection<ESXRecord, IdAccessorT>::getRecord(int index) const
{
    return records.at(index);
}

template<typename ESXRecord, typename IdAccessorT>
const Record<ESXRecord>& Collection<ESXRecord, IdAccessorT>::getRecord(const QString& id) const
{
    int index = getIndex(id);
    return records.at(index);
}

template<typename ESXRecord, typename IdAccessorT>
Record<ESXRecord>& Collection<ESXRecord, IdAccessorT>::getRecord(int index)
{
    return records[index];
}

template<typename ESXRecord, typename IdAccessorT>
Record<ESXRecord>& Collection<ESXRecord, IdAccessorT>::getRecord(const QString& id)
{
    int index = getIndex(id);
    return records[index];
}

template<typename ESXRecord, typename IdAccessorT>
int Collection<ESXRecord, IdAccessorT>::getAppendIndex(const QString& id, CkId::Type type) const
{
    return static_cast<int>(records.size());
}

template<typename ESXRecord, typename IdAccessorT>
void Collection<ESXRecord, IdAccessorT>::setRecord(int index, const Record<ESXRecord>& record)
{
    if (IdAccessorT().getId(records[index].get()).toLower() != IdAccessorT().getId(record.get()).toLower())
    {
        throw std::runtime_error("Attempted to change the ID of a record");
    }

    records[index] = record;
}

template<typename ESXRecord, typename IdAccessorT>
void Collection<ESXRecord, IdAccessorT>::renameRecord(int index, const QString& newEditorId)
{
    if (index < 0 || index >= records.size())
    {
        return;
    }

    QString oldId = IdAccessorT().getId(records[index].get()).toLower();
    QString newIdLower = newEditorId.toLower();

    IdAccessorT().setId(records[index].get(), newEditorId);
    records[index].setModified(records[index].get());

    indexes.remove(oldId);
    indexes.insert(newIdLower, index);
}

template<typename ESXRecord, typename IdAccessorT>
bool Collection<ESXRecord, IdAccessorT>::reorderRows(int baseIndex, const QVector<int>& newOrder)
{
    return reorderRowsImp(baseIndex, newOrder);
}

// Deep-copy a single record for move/drag-drop reordering (see BaseCollection::cloneRecord).
// Reuses Record<ESXRecord>::clone(), which returns a deep copy as std::unique_ptr<BaseRecord>.
template<typename ESXRecord, typename IdAccessorT>
std::unique_ptr<BaseRecord> Collection<ESXRecord, IdAccessorT>::cloneRecordAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(records.size()))
        return nullptr;

    const Record<ESXRecord>& src = records.at(index);

    if (src.isErased())
        return nullptr;

    return src.clone();
}

#endif // COLLECTION_H
