#ifndef EDITRECORDCOMMAND_H
#define EDITRECORDCOMMAND_H

#include "command.hpp"
#include "../world/record.hpp"
#include "../world/collection.hpp"

template<typename ESXRecord>
class EditRecordCommand : public Command
{
public:
    EditRecordCommand(Collection<ESXRecord>* collection, int index,
                      const ESXRecord& originalState, const ESXRecord& newState,
                      const QString& description = QString())
        : mCollection(collection), mIndex(index), mOriginalState(originalState), mNewState(newState)
    {
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
        if (!mCollection)
        {
            return;
        }
        Record<ESXRecord>& rec = mCollection->getRecord(mIndex);
        rec.setModified(mNewState);
    }

    void undo() override
    {
        if (!mCollection)
        {
            return;
        }
        Record<ESXRecord>& rec = mCollection->getRecord(mIndex);
        rec.setModified(mOriginalState);
    }

    QString name() const override
    {
        return mName;
    }

private:
    Collection<ESXRecord>* mCollection;
    int mIndex;
    ESXRecord mOriginalState;
    ESXRecord mNewState;
    QString mName;
};

#endif // EDITRECORDCOMMAND_H
