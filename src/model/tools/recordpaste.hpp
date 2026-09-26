#ifndef RECORDPASTE_HPP
#define RECORDPASTE_HPP

#include <QString>
#include <memory>

class BaseCollection;
class BaseRecord;
class IdTable;
class UndoStack;

namespace openck {

/// Copies the record at (coll, recordIndex) into `out`, stamped with the given
/// editor ID and form ID. The copy is a deep clone, so every field travels
/// rather than the hand-listed subset the clipboard's JSON map carried.
/// Returns false when the collection's record type is not one this supports.
bool copyRecordForPaste(BaseCollection* coll, int recordIndex,
                        const QString& editorId, quint32 formId,
                        std::unique_ptr<BaseRecord>& out);

/// Adds a copy of the record at (coll, recordIndex) to the same collection
/// under a new editor ID and form ID, through the document's undo stack.
/// Returns false when the type is unsupported, the index is out of range, or
/// there is no undo stack to record the add on.
bool addRecordCopyThroughUndo(BaseCollection* coll, int recordIndex,
                              const QString& editorId, quint32 formId,
                              IdTable* table, UndoStack* stack,
                              const QString& description = QString());

} // namespace openck

#endif // RECORDPASTE_HPP
