#include "deleterecordcommand.hpp"

DeleteRecordCommand::DeleteRecordCommand(IdTable* model, BaseCollection* collection, int index, const QString& description)
    : mModel(model),
      mCollection(collection),
      mIndex(index),
      mDeletedRecord(nullptr)
{
    if (mCollection && mIndex >= 0
        && mIndex < static_cast<int>(mCollection->count()))
    {
        mDeletedRecord = mCollection->getRecord(mIndex).clone();
    }

    if (!description.isEmpty())
    {
        mDescription = description;
    }
    else
    {
        mDescription = "Delete record";
    }
}

void DeleteRecordCommand::execute()
{
    if (!mCollection || mIndex < 0 || mIndex >= static_cast<int>(mCollection->count()))
    {
        return;
    }
    mCollection->removeRows(mIndex, 1);
    if (mModel) {
        mModel->dataChanged(mModel->index(mIndex, 0),
            mModel->index(mModel->rowCount() - 1, mModel->columnCount() - 1));
    }
}

void DeleteRecordCommand::undo()
{
    if (!mCollection || !mDeletedRecord
        || mIndex < 0 || mIndex > static_cast<int>(mCollection->count()))
    {
        return;
    }
    mCollection->insertRecord(*mDeletedRecord, mIndex);
    if (mModel) {
        // Inserted at mIndex shifts all rows >= mIndex down by one, so mark
        // from mIndex to the end as changed.
        mModel->dataChanged(mModel->index(mIndex, 0),
            mModel->index(mModel->rowCount() - 1, mModel->columnCount() - 1));
    }
}

QString DeleteRecordCommand::name() const
{
    return mDescription;
}
