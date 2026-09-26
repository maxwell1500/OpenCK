#ifndef RECORDEDITSESSION_HPP
#define RECORDEDITSESSION_HPP

#include "../../libs/components/formcomponents.hpp"
#include "../../model/tools/editrecordcommand.hpp"
#include "../../model/tools/undostack.hpp"
#include "../../model/world/collection.hpp"
#include "../../model/world/record.hpp"

#include <QString>

#include <memory>

namespace openck {

/// One record's edit session.
///
/// The session owns a working copy of the record. The dialog's property grid
/// and any custom data widget edit that copy and nothing else, so no route can
/// mutate a base record without an undo entry: commit() is the only path that
/// writes to the live record, and it always goes through the document's undo
/// stack. Closing or cancelling the dialog discards the working copy.
///
/// This replaces handing custom data widgets the live record pointer, which
/// let widgets such as INFO, QUST and WRLD write straight into the base record
/// with no undo and no way to cancel.
class RecordEditSession
{
public:
    virtual ~RecordEditSession() = default;

    /// The working copy. Never the live record.
    virtual void* workingRecord() = 0;

    /// The components of the working copy, or nullptr for record types that
    /// have no component set. Owned by the session.
    openck::FormComponents* workingComponents() const { return m_components; }

    /// True when the working copy differs from the record as it stood at open.
    virtual bool hasChanges() const = 0;

    /// Writes the working copy to the live record through the document's undo
    /// stack. Returns true when something was actually written. Idempotent.
    virtual bool commit() = 0;

    /// Releases the working copy. Idempotent, and safe to call after commit().
    virtual void discard() { m_finished = true; }

    bool isFinished() const { return m_finished; }

protected:
    openck::FormComponents* m_components = nullptr;
    bool m_finished = false;
};

/// Typed session over Collection<RecordType>.
template <typename RecordType>
class TypedRecordEditSession : public RecordEditSession
{
public:
    TypedRecordEditSession(Collection<RecordType>* collection, int index,
                           UndoStack* undoStack, const QString& description)
        : mCollection(collection)
        , mIndex(index)
        , mUndoStack(undoStack)
        , mDescription(description)
    {
        if (mCollection && mIndex >= 0 && mIndex < mCollection->size())
            m_before = mCollection->getRecord(mIndex).get();
        m_working = m_before;
        if constexpr (HasFormComponents<RecordType>::value)
            m_components = &m_working.components;
    }

    void* workingRecord() override { return &m_working; }

    bool hasChanges() const override { return m_working != m_before; }

    bool commit() override
    {
        if (m_finished) return false;
        m_finished = true;
        if (!mCollection || mIndex < 0 || mIndex >= mCollection->size())
            return false;
        if (m_working == m_before)
            return false;
        if (mUndoStack)
        {
            auto* command = new EditRecordCommand<RecordType>(
                mCollection, mIndex, m_before, m_working, mDescription);
            if (command->hasChanged())
                mUndoStack->push(command);
            else
                delete command;
        }
        else
        {
            mCollection->getRecord(mIndex).setModified(m_working);
        }
        return true;
    }

private:
    Collection<RecordType>* mCollection = nullptr;
    int mIndex = -1;
    UndoStack* mUndoStack = nullptr;
    QString mDescription;
    RecordType m_before;
    RecordType m_working;
};

} // namespace openck

#endif // RECORDEDITSESSION_HPP
