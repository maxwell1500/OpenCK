#include "addrecordcommand.hpp"

AddRecordCommand::AddRecordCommand(IdTable* model, BaseCollection* collection, int index, const BaseRecord& record, const QString& description)
    : mModel(model),
      mCollection(collection),
      mIndex(index),
      mRecord(record.clone())
{
    if (!description.isEmpty())
    {
        mDescription = description;
    }
    else
    {
        mDescription = "Add record";
    }
}

void AddRecordCommand::execute()
{
    mCollection->appendRecord(*mRecord);
    mModel->dataChanged(mModel->index(mIndex, 0),
        mModel->index(mIndex, mModel->columnCount() - 1));
}

void AddRecordCommand::undo()
{
    mCollection->removeRows(mIndex, 1);
    mModel->dataChanged(mModel->index(0, 0),
        mModel->index(mModel->rowCount() - 1, mModel->columnCount() - 1));
}

QString AddRecordCommand::name() const
{
    return mDescription;
}
