#ifndef RECORDEDITCOMMAND_H
#define RECORDEDITCOMMAND_H

#include "command.hpp"
#include "../world/record.hpp"
#include "../world/collection.hpp"

template<typename ESXRecord>
class RecordEditCommand : public Command
{
public:
    RecordEditCommand(Collection<ESXRecord>& collection, int index,
                      const ESXRecord& newState,
                      const QString& description = QString())
        : mCollection(collection), mIndex(index), mNewState(newState)
    {
        const Record<ESXRecord>& rec = collection.getRecord(index);
        mOriginalState = rec.get();

        if (!description.isEmpty())
        {
            mName = description;
        }
        else
        {
            QString id = newState.editorId.isEmpty() ? "(unnamed)" : newState.editorId;
            mName = "Edit record: " + id;
        }
    }

    bool hasChanged() const
    {
        return mOriginalState != mNewState;
    }

    void execute() override
    {
        Record<ESXRecord>& rec = mCollection.getRecord(mIndex);
        rec.setModified(mNewState);
    }

    void undo() override
    {
        Record<ESXRecord>& rec = mCollection.getRecord(mIndex);
        rec.setModified(mOriginalState);
    }

    QString name() const override
    {
        return mName;
    }

private:
    Collection<ESXRecord>& mCollection;
    int mIndex;
    ESXRecord mOriginalState;
    ESXRecord mNewState;
    QString mName;
};

#endif // RECORDEDITCOMMAND_H
