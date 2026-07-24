#include "idtable.hpp"
#include "../tools/setcolumncommand.hpp"
#include "../tools/addrecordcommand.hpp"
#include "../tools/deleterecordcommand.hpp"
#include "../tools/undostack.hpp"
#include "basecolumn.hpp"
#include "baseidtable.hpp"
#include "record.hpp"

#include <memory>
#include <vector>

IdTable::IdTable(BaseCollection* idCollection, unsigned int features, QObject* parent) :
    BaseIdTable(features, parent),
    collection(idCollection),
    mUndoStack(nullptr)
{
}


IdTable::~IdTable()
{
}

int IdTable::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return collection->size();
}

int IdTable::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return collection->getColumns();
}


QVariant IdTable::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.column() < 0)
        return QVariant();

    if (role == BaseColumn::Role_Display)
        return QVariant(collection->getColumn(index.column()).displayType);
    if (role == BaseColumn::Role_ColumnId)
        return QVariant(getColumnId(index.column()));
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();
    if (role == Qt::EditRole && !collection->getColumn(index.column()).isEditable())
        return QVariant();

    return collection->getData(index.row(), index.column());
}

QVariant IdTable::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical)
        return QVariant();
    if (orientation != Qt::Horizontal)
        throw std::logic_error("Unknown header orientation invalid");

    if (role == Qt::DisplayRole)
        return collection->getColumn(section).getTitle();
    if (role == BaseColumn::Role_Flags)
        return collection->getColumn(section).flags;
    if (role == BaseColumn::Role_Display)
        return collection->getColumn(section).displayType;
    if (role == BaseColumn::Role_ColumnId)
        return getColumnId(section);

    return QVariant();
}


bool IdTable::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (collection->getColumn(index.column()).isEditable() && role == Qt::EditRole)
    {
        if (mUndoStack)
        {
            QVariant oldValue = collection->getData(index.row(), index.column());
            if (oldValue != value)
            {
                SetColumnCommand* cmd = new SetColumnCommand(this, collection, index.row(), index.column(), value);
                mUndoStack->push(cmd);
            }
        }

        collection->setData(index.row(), index.column(), value);

        int stateColumn = searchColumnIndex(ColumnId::ColumnId_Modification);
        if (stateColumn != -1)
        {
            if (index.column() == stateColumn)
                emit dataChanged(this->index(index.row(), 0), this->index(index.row(), columnCount(index.parent())));
            else
            {
                emit dataChanged(index, index);
                QModelIndex stateIndex = this->index(index.row(), stateColumn);
                emit dataChanged(stateIndex, stateIndex);
            }
        }
        else
            emit dataChanged(index, index);

        return true;
    }
    return false;
}

Qt::ItemFlags IdTable::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::ItemFlags{};
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (collection->getColumn(index.column()).isUserEditable())
        flags |= Qt::ItemIsEditable;
    return flags;
}


bool IdTable::removeRows(int row, int count, const QModelIndex& parent)
{
    if (parent.isValid())
        return false;

    if (mUndoStack && count == 1)
    {
        DeleteRecordCommand* cmd = new DeleteRecordCommand(this, collection, row);
        mUndoStack->push(cmd);
    }

    beginRemoveRows(parent, row, row + count - 1);
    collection->removeRows(row, count);
    endRemoveRows();
    return true;
}

QModelIndex IdTable::index(int row, int column, const QModelIndex& parent) const
{
    if (parent.isValid())
        return QModelIndex();
    if (row < 0 || row >= collection->size())
        return QModelIndex();
    if (column < 0 || column >= collection->getColumns())
        return QModelIndex();
    return createIndex(row, column);
}

QModelIndex IdTable::parent(const QModelIndex& index) const
{
    return QModelIndex();
}


void IdTable::addRecord(const QString& id, CkId::Type type)
{
    int index = collection->getAppendIndex(id, type);
    beginInsertRows(QModelIndex(), index, index);
    collection->appendBlankRecord(id, type);
    if (mUndoStack)
    {
        const BaseRecord& record = collection->getRecord(index);
        AddRecordCommand* cmd = new AddRecordCommand(this, collection, index, record);
        mUndoStack->push(cmd);
    }
    endInsertRows();
}

void IdTable::addRecordWithData(const QString& id, const QMap<int, QVariant>& data, CkId::Type type)
{
    int index = collection->getAppendIndex(id, type);
    beginInsertRows(QModelIndex(), index, index);
    collection->appendBlankRecord(id, type);
    for (auto it = data.begin(); it != data.end(); ++it)
        collection->setData(index, it.key(), it.value());
    if (mUndoStack)
    {
        const BaseRecord& record = collection->getRecord(index);
        AddRecordCommand* cmd = new AddRecordCommand(this, collection, index, record);
        mUndoStack->push(cmd);
    }
    endInsertRows();
}

void IdTable::cloneRecord(const QString& src, const QString& dest, CkId::Type type)
{
    int index = collection->getAppendIndex(dest);
    beginInsertRows(QModelIndex(), index, index);
    collection->cloneRecord(src, dest, type);
    endInsertRows();
}

bool IdTable::touchRecord(const QString& id)
{
    bool changed = collection->touchRecord(id);
    int row = collection->getIndex(id);
    int column = collection->searchColumnIndex(ColumnId::ColumnId_RecordType);
    if (changed && column != -1)
    {
        QModelIndex modelIndex = index(row, column);
        emit dataChanged(modelIndex, modelIndex);
    }
    return changed;
}

QString IdTable::getId(int row) const
{
    return collection->getId(row);
}

QModelIndex IdTable::getModelIndex(const QString& id, int column) const
{
    int row = collection->searchId(id);
    if (row != -1)
        return index(row, column);
    return QModelIndex();
}


void IdTable::setRecord(const QString& id, const BaseRecord& record, CkId::Type type)
{
    int index = collection->searchId(id);
    if (index == -1)
    {
        index = collection->getAppendIndex(id, type);
        beginInsertRows(QModelIndex(), index, index);
        collection->appendRecord(record, type);
        endInsertRows();
    }
}

const BaseRecord& IdTable::getRecord(const QString& id) const
{
    return collection->getRecord(id);
}

int IdTable::searchColumnIndex(ColumnId id) const
{
    return collection->searchColumnIndex(id);
}

int IdTable::findColumnIndex(ColumnId id) const
{
    return collection->findColumnIndex(id);
}

void IdTable::reorderRows(int baseIndex, const QVector<int>& newOrder)
{
    if (!newOrder.empty())
    {
        if (collection->reorderRows(baseIndex, newOrder))
            emit dataChanged(index(baseIndex, 0), index(baseIndex + newOrder.size() - 1, collection->getColumns() - 1));
    }
}


bool IdTable::moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                       const QModelIndex& destParent, int destRow)
{
    Q_UNUSED(sourceParent);
    Q_UNUSED(destParent);

    if (count <= 0)
        return false;

    int actualDest = destRow;
    if (actualDest < 0)
        actualDest = collection->size();

    const int sourceEnd = sourceRow + count;

    // Clone the records we are about to move BEFORE removing them from the collection.
    // cloneRecord() is the single type-safe hook: it returns a deep-copy clone whose ownership
    // transfers to us (stored in unique_ptr below), so the originals can be safely removed.
    std::vector<std::unique_ptr<BaseRecord>> savedRecords;
    savedRecords.reserve(count);
    for (int i = 0; i < count; ++i)
    {
        auto clone = collection->cloneRecordAt(sourceRow + i);
        if (clone)
            savedRecords.emplace_back(std::move(clone));
        else
        {
            // Nothing to clone (e.g. erased record); insert a placeholder so indices align.
            savedRecords.emplace_back(nullptr);
        }
    }

    beginMoveRows(sourceParent, sourceRow, sourceEnd - 1, destParent, actualDest);

    // Remove the originals (back to front so indices stay valid).
    for (int i = count - 1; i >= 0; --i)
        collection->removeRows(sourceRow + i, 1);

    // Re-insert the clones at their new positions.
    for (int i = 0; i < count; ++i)
    {
        if (!savedRecords[i])
            continue; // erased record: already removed, nothing to re-insert

        int insertPos;
        if (actualDest > sourceEnd)
            insertPos = actualDest - count + i;
        else
            insertPos = actualDest + i;

        if (insertPos < 0) insertPos = 0;
        if (insertPos > collection->size()) insertPos = collection->size();

        collection->insertRecord(*savedRecords[i], insertPos);
    }

    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
    endMoveRows();
    return true;
}


QPair<CkId, QString> IdTable::view(int row) const
{
    QString id;
    QString hint;
    if (getFeatures() & Feature_ViewId)
    {
        int column = collection->searchColumnIndex(ColumnId::ColumnId_Id);
        if (column != -1)
        {
            id = collection->getData(row, column).toString();
            hint = "c:" + id;
        }
    }
    if (id.isEmpty())
        return QPair<CkId, QString>(CkId::Type_None, "");
    return QPair<CkId, QString>(CkId(id), hint);
}

bool IdTable::isDeleted(const QString& id) const
{
    return getRecord(id).isDeleted();
}

int IdTable::getColumnId(int column) const
{
    return collection->getColumn(column).getId();
}

BaseCollection* IdTable::idCollection() const
{
    return collection;
}

void IdTable::setUndoStack(UndoStack* stack)
{
    mUndoStack = stack;
}

