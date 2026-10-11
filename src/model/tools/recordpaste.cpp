#include "recordpaste.hpp"

#include "addrecordcommand.hpp"
#include "undostack.hpp"
#include "../world/basecollection.hpp"
#include "../world/idtable.hpp"

namespace openck {

bool copyRecordForPaste(BaseCollection* coll, int recordIndex,
                        const QString& editorId, quint32 formId,
                        std::unique_ptr<BaseRecord>& out)
{
    if (!coll)
        return false;
    out = coll->copyRecordForPaste(recordIndex, editorId, formId);
    return out != nullptr;
}

bool addRecordCopyThroughUndo(BaseCollection* coll, int recordIndex,
                              const QString& editorId, quint32 formId,
                              IdTable* table, UndoStack* stack,
                              const QString& description)
{
    if (!coll || !table || !stack)
        return false;
    std::unique_ptr<BaseRecord> copy = coll->copyRecordForPaste(recordIndex, editorId, formId);
    if (!copy)
        return false;
    const int targetIndex = coll->getAppendIndex(editorId, CkId::Type_None);
    stack->push(new AddRecordCommand(table, coll, targetIndex, *copy, description));
    return true;
}

} // namespace openck
