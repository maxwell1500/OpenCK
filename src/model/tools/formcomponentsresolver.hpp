#ifndef FORMCOMPONENTSRESOLVER_HPP
#define FORMCOMPONENTSRESOLVER_HPP

#include "../../view/window/recordeditsession.hpp"

#include <memory>

class BaseCollection;
class UndoStack;
namespace openck { class FormComponents; }

/// Resolves the FormComponents + record pointer for a record inside a
/// collection, for record types that participate in the component
/// architecture. Owned by one shared .cpp so both the Object Window and
/// the Search dialog can open any record through the generic form dialog.
/// Returns false if the type has no component-based record struct.
///
/// The returned pointers address the live record. Prefer
/// resolveEditSession() for editing: it hands back a session over a working
/// copy so the dialog cannot write to the base record outside an undo command.
bool resolveComponents(BaseCollection* coll, int recordIndex,
                       openck::FormComponents*& components, void*& recordPtr);

/// Builds a typed RecordEditSession for a record inside a collection, using
/// the same record-type list as resolveComponents(). The caller supplies the
/// document's undo stack and a description for the undo entry. Returns false
/// if the collection's record type is not component-based.
bool resolveEditSession(BaseCollection* coll, int recordIndex,
                        UndoStack* undoStack, const QString& description,
                        std::unique_ptr<openck::RecordEditSession>& session);

#endif // FORMCOMPONENTSRESOLVER_HPP