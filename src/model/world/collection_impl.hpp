#ifndef COLLECTION_IMPL_HPP
#define COLLECTION_IMPL_HPP

#include "collection.hpp"
#include "../tools/editrecordcommand.hpp"
#include "../tools/deleterecordcommandbase.hpp"
#include "../tools/macrocommand.hpp"
#include "../tools/undostack.hpp"
#include "../../../libs/files/esm/esmwriter.hpp"
#include "../../../libs/files/log/logger.hpp"

// ─── IRecordCollection overrides ──────────────────────────────────────────

template<typename ESXRecord, typename IdAccessorT>
quint32 Collection<ESXRecord, IdAccessorT>::getFormId(int index) const
{
    if constexpr (HasFormIdField<ESXRecord>::value)
    {
        return records.at(index).get().formId;
    }
    else
    {
        return 0;
    }
}

template<typename ESXRecord, typename IdAccessorT>
void Collection<ESXRecord, IdAccessorT>::setFormId(int index, quint32 formId)
{
    if constexpr (HasFormIdField<ESXRecord>::value)
    {
        if (index >= 0 && index < records.size())
        {
            Record<ESXRecord>& rec = records[index];
            if (rec.state == State_Base || rec.state == State_Deleted)
            {
                // Promote the record to Modified so the new ID is persisted.
                rec.modifiedRecord = rec.baseRecord;
                rec.modifiedRecord.formId = formId;
                rec.state = State_Modified;
            }
            else
            {
                rec.get().formId = formId;
                if (rec.state == State_Modified)
                    rec.modifiedRecord.formId = formId;
            }
        }
    }
}

template<typename ESXRecord, typename IdAccessorT>
bool Collection<ESXRecord, IdAccessorT>::containsFormId(quint32 formId) const
{
    if constexpr (HasFormIdField<ESXRecord>::value)
    {
        for (const auto& rec : records)
        {
            if (!rec.isErased() && rec.get().formId == formId)
                return true;
        }
        return false;
    }
    else
    {
        return false;
    }
}

template<typename ESXRecord, typename IdAccessorT>
bool Collection<ESXRecord, IdAccessorT>::isRecordModified(int index) const
{
    return records.at(index).state == State_Modified || records.at(index).state == State_ModifiedOnly;
}

template<typename ESXRecord, typename IdAccessorT>
void Collection<ESXRecord, IdAccessorT>::saveModifiedRecords(ESMWriter& writer, uint32_t recordType) const
{
    for (const auto& record : records)
    {
        if (record.state == State_Modified || record.state == State_ModifiedOnly)
        {
            RecHeader recHeader;
            if constexpr (HasFormIdField<ESXRecord>::value)
                recHeader.id = record.get().formId;
            writer.startRecord(static_cast<NAME>(recordType), recHeader);
            record.get().save(writer);
            writer.endRecord();
        }
        else if (record.state == State_Deleted)
        {
            RecHeader delHeader;
            if constexpr (HasFormIdField<ESXRecord>::value)
                delHeader.id = record.get().formId;
            writer.startRecord(static_cast<NAME>(recordType), delHeader);
            writer.startSubRecord(static_cast<NAME>('DELE'));
            writer.endSubRecord();
            writer.endRecord();
        }
    }

    for (quint32 formId : mDeletedFormIds)
    {
        bool stillExists = false;
        if constexpr (HasFormIdField<ESXRecord>::value)
        {
            for (const auto& record : records)
            {
                if (!record.isErased() && record.get().formId == formId)
                {
                    stillExists = true;
                    break;
                }
            }
        }
        if (!stillExists)
        {
            RecHeader delHeader;
            delHeader.id = formId;
            writer.startRecord(static_cast<NAME>(recordType), delHeader);
            writer.startSubRecord(static_cast<NAME>('DELE'));
            writer.endSubRecord();
            writer.endRecord();
        }
    }
}

// ─── Undo-aware operations ────────────────────────────────────────────────

template<typename ESXRecord, typename IdAccessorT>
bool Collection<ESXRecord, IdAccessorT>::removeRecordWithUndo(const QString& id, UndoStack* undoStack)
{
    int index = searchId(id);
    if (index == -1) return false;

    if constexpr (HasFormIdField<ESXRecord>::value)
    {
        quint32 formId = records[index].get().formId;
        if (formId != 0 && !mDeletedFormIds.contains(formId))
        {
            mDeletedFormIds.append(formId);
        }
    }

    QString description = QString("Delete record: %1").arg(getId(index));
    if (undoStack)
    {
        undoStack->push(new DeleteRecordCommandBase(this, index, description));
    }
    else
    {
        removeRows(index, 1);
    }
    return true;
}

template<typename ESXRecord, typename IdAccessorT>
bool Collection<ESXRecord, IdAccessorT>::cloneRecordWithUndo(const QString& src, const QString& dest, UndoStack* undoStack)
{
    int srcIndex = searchId(src);
    if (srcIndex == -1) return false;

    ESXRecord originalState = records[srcIndex].get();

    try
    {
        cloneRecord(src, dest, CkId::Type_None);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(QString("cloneRecord failed: %1").arg(e.what()));
        return false;
    }

    int destIndex = searchId(dest);
    if (destIndex == -1) return false;

    ESXRecord newState = records[destIndex].get();

    if (undoStack)
    {
        QString originalDesc = QString("Mark '%1' as original").arg(src);
        QString newDesc = QString("Mark '%1' as new clone").arg(dest);
        undoStack->push(new EditRecordCommand<ESXRecord>(this, srcIndex, originalState, originalState, originalDesc));
        undoStack->push(new EditRecordCommand<ESXRecord>(this, destIndex, newState, newState, newDesc));
    }

    return true;
}

template<typename ESXRecord, typename IdAccessorT>
void Collection<ESXRecord, IdAccessorT>::batchCloneWithUndo(const QVector<QString>& srcIds, const QVector<QString>& destIds, UndoStack* undoStack)
{
    if (!undoStack || srcIds.size() != destIds.size()) return;

    MacroCommand* macro = new MacroCommand(QString("Batch clone %1 record(s)").arg(srcIds.size()));
    int commandCount = 0;

    for (int i = 0; i < srcIds.size(); ++i)
    {
        int srcIndex = searchId(srcIds[i]);
        if (srcIndex == -1) continue;

        ESXRecord originalState = records[srcIndex].get();

        try
        {
            cloneRecord(srcIds[i], destIds[i], CkId::Type_None);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(QString("batchClone record failed: %1").arg(e.what()));
            continue;
        }

        int destIndex = searchId(destIds[i]);
        if (destIndex == -1) continue;

        ESXRecord newState = records[destIndex].get();

        QString originalDesc = QString("Mark '%1' as original").arg(srcIds[i]);
        QString newDesc = QString("Mark '%1' as new clone").arg(destIds[i]);

        macro->addCommand(new EditRecordCommand<ESXRecord>(this, srcIndex, originalState, originalState, originalDesc));
        macro->addCommand(new EditRecordCommand<ESXRecord>(this, destIndex, newState, newState, newDesc));
        commandCount += 2;
    }

    if (commandCount > 0)
    {
        undoStack->push(macro);
    }
    else
    {
        delete macro;
    }
}

template<typename ESXRecord, typename IdAccessorT>
void Collection<ESXRecord, IdAccessorT>::batchSetEditorIdWithUndo(const QVector<QString>& srcIds, const QString& newEditorId, UndoStack* undoStack)
{
    if (!undoStack || srcIds.isEmpty()) return;

    MacroCommand* macro = new MacroCommand(QString("Batch set EditorID for %1 record(s)").arg(srcIds.size()));
    int commandCount = 0;

    for (const QString& srcId : srcIds)
    {
        int index = searchId(srcId);
        if (index == -1) continue;

        ESXRecord originalState = records[index].get();
        ESXRecord newState = originalState;
        newState.editorId = newEditorId;

        QString desc = QString("Set EditorID to '%1' for '%2'").arg(newEditorId, srcId);
        macro->addCommand(new EditRecordCommand<ESXRecord>(this, index, originalState, newState, desc));
        commandCount++;
    }

    if (commandCount > 0)
    {
        undoStack->push(macro);
    }
    else
    {
        delete macro;
    }
}

#endif // COLLECTION_IMPL_HPP
