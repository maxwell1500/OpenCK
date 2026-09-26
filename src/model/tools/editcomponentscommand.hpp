#ifndef EDITCOMPONENTSCOMMAND_H
#define EDITCOMPONENTSCOMMAND_H

#include "command.hpp"
#include "../world/basecollection.hpp"
#include "../world/record.hpp"

#include <memory>

/// Undoable replacement of a record's components, for edit routes that only
/// hold a BaseCollection and cannot name the concrete record struct.
///
/// Snapshot/restore goes through BaseCollection::cloneRecordAt() and
/// BaseRecord::activeComponents(), so this works for any component-based
/// record type without a per-type template. Routes that do know the type
/// should keep using EditRecordCommand<T>, which copies the whole record
/// rather than just its components.
class EditComponentsCommand : public Command
{
public:
    EditComponentsCommand(BaseCollection* collection, int index,
                          const openck::FormComponents& edited,
                          const QString& description = QString())
        : mCollection(collection), mIndex(index), mEdited(edited)
    {
        mName = description.isEmpty() ? QStringLiteral("Edit record") : description;
    }

    /// Takes the pre-edit snapshot. Must be called before the live record's
    /// components are overwritten, otherwise there is nothing to undo to.
    /// Returns false when the record has no components, in which case the
    /// command does nothing and the caller should not push it.
    bool captureBefore()
    {
        if (!mCollection || mIndex < 0 || mIndex >= mCollection->size())
            return false;
        mBefore = mCollection->cloneRecordAt(mIndex);
        if (!mBefore)
            return false;
        const openck::FormComponents* live = mBefore->activeComponents();
        if (!live)
        {
            mBefore.reset();
            return false;
        }
        return *live != mEdited;
    }

    bool hasChanged() const
    {
        return mBefore && mBefore->activeComponents()
            && *mBefore->activeComponents() != mEdited;
    }

    void execute() override
    {
        applyComponents(mEdited);
    }

    void undo() override
    {
        if (!mBefore) return;
        const openck::FormComponents* original = mBefore->activeComponents();
        if (original)
            applyComponents(*original);
    }

    QString name() const override { return mName; }

private:
    void applyComponents(const openck::FormComponents& components)
    {
        if (!mCollection || mIndex < 0 || mIndex >= mCollection->size())
            return;
        // Copy the current record, swap the components, and write the whole
        // record back, so the base record is only ever mutated through a
        // complete, undoable replacement.
        std::unique_ptr<BaseRecord> editedRecord = mCollection->cloneRecordAt(mIndex);
        if (!editedRecord) return;
        openck::FormComponents* target = editedRecord->activeComponents();
        if (!target) return;
        *target = components;
        mCollection->replace(mIndex, *editedRecord);
    }

    BaseCollection* mCollection = nullptr;
    int mIndex = -1;
    openck::FormComponents mEdited;
    std::unique_ptr<BaseRecord> mBefore;
    QString mName;
};

#endif // EDITCOMPONENTSCOMMAND_H
