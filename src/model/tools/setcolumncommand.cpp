#include "setcolumncommand.hpp"
#include "../world/basecolumn.hpp"

SetColumnCommand::SetColumnCommand(IdTable* model, BaseCollection* collection, int recordIndex, int column, const QVariant& newValue, const QString& description)
    : mModel(model),
      mCollection(collection),
      mRecordIndex(recordIndex),
      mColumn(column),
      mNewValue(newValue)
{
    mOldValue = mCollection->getData(mRecordIndex, mColumn);
    
    if (!description.isEmpty())
    {
        mDescription = description;
    }
    else
    {
        QString colName = "unknown";
        if (collection)
        {
            const BaseColumn& col = collection->getColumn(column);
            colName = col.getTitle();
        }
        mDescription = QString("Change %1 to '%2'").arg(colName).arg(newValue.toString());
    }
}

void SetColumnCommand::execute()
{
    if (!mCollection || !mModel)
    {
        return;
    }
    mCollection->setData(mRecordIndex, mColumn, mNewValue);
    notifyModel();
}

void SetColumnCommand::undo()
{
    if (!mCollection || !mModel)
    {
        return;
    }
    mCollection->setData(mRecordIndex, mColumn, mOldValue);
    notifyModel();
}

void SetColumnCommand::notifyModel()
{
    if (!mModel)
    {
        return;
    }
    int stateColumn = mModel->searchColumnIndex(ColumnId::ColumnId_Modification);
    if (stateColumn != -1)
    {
        if (mColumn == stateColumn)
        {
            mModel->dataChanged(mModel->index(mRecordIndex, 0),
                mModel->index(mRecordIndex, mModel->columnCount()));
        }
        else
        {
            mModel->dataChanged(mModel->index(mRecordIndex, mColumn), mModel->index(mRecordIndex, mColumn));

            QModelIndex stateIndex = mModel->index(mRecordIndex, stateColumn);
            mModel->dataChanged(stateIndex, stateIndex);
        }
    }
    else
    {
        mModel->dataChanged(mModel->index(mRecordIndex, mColumn), mModel->index(mRecordIndex, mColumn));
    }
}

QString SetColumnCommand::name() const
{
    return mDescription;
}
