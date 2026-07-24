#ifndef TEMPLATEDELETERECORDCOMMAND_H
#define TEMPLATEDELETERECORDCOMMAND_H

#include "command.hpp"
#include "../world/record.hpp"
#include "../world/collection.hpp"

template<typename ESXRecord>
class DeleteRecordCommand : public Command
{
public:
    DeleteRecordCommand(Collection<ESXRecord>* collection, int index, const QString& description = QString())
        : mCollection(collection), mIndex(index)
    {
        if (!description.isEmpty())
        {
            mName = description;
        }
        else
        {
            mName = "Delete record";
        }
    }

    void execute() override
    {
        if (!mCollection || mIndex < 0 || mIndex >= static_cast<int>(mCollection->size()))
        {
            return;
        }
        mDeletedRecord = std::make_unique<ESXRecord>(mCollection->getRecord(mIndex).get());
        mCollection->removeRows(mIndex, 1);
    }

    void undo() override
    {
        if (!mCollection || !mDeletedRecord)
        {
            return;
        }
        mCollection->add(*mDeletedRecord);
    }

    QString name() const override
    {
        return mName;
    }

private:
    Collection<ESXRecord>* mCollection;
    int mIndex;
    std::unique_ptr<ESXRecord> mDeletedRecord;
    QString mName;
};

#endif // TEMPLATEDELETERECORDCOMMAND_H
