#ifndef DELETERECORDCOMMANDBASE_HPP
#define DELETERECORDCOMMANDBASE_HPP

#include "command.hpp"
#include "../world/record.hpp"
#include "../world/basecollection.hpp"

#include <memory>

class DeleteRecordCommandBase : public Command
{
public:
    DeleteRecordCommandBase(BaseCollection* collection, int index, const QString& description = QString())
        : mCollection(collection), mIndex(index)
    {
        mName = description.isEmpty() ? "Delete record" : description;
    }

    void execute() override
    {
        if (!mCollection || mIndex < 0 || mIndex >= mCollection->size())
        {
            return;
        }
        mDeletedRecord = mCollection->getRecord(mIndex).clone();
        mCollection->removeRows(mIndex, 1);
    }

    void undo() override
    {
        if (!mCollection || !mDeletedRecord)
        {
            return;
        }
        mCollection->insertRecord(*mDeletedRecord, mIndex);
    }

    QString name() const override
    {
        return mName;
    }

private:
    BaseCollection* mCollection;
    int mIndex;
    std::unique_ptr<BaseRecord> mDeletedRecord;
    QString mName;
};

#endif // DELETERECORDCOMMANDBASE_HPP
