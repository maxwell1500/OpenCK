#ifndef FORMCOMPONENTSRESOLVER_HPP
#define FORMCOMPONENTSRESOLVER_HPP

class BaseCollection;
namespace openck { class FormComponents; }

/// Resolves the FormComponents + record pointer for a record inside a
/// collection, for record types that participate in the component
/// architecture. Owned by one shared .cpp so both the Object Window and
/// the Search dialog can open any record through the generic form dialog.
/// Returns false if the type has no component-based record struct.
bool resolveComponents(BaseCollection* coll, int recordIndex,
                       openck::FormComponents*& components, void*& recordPtr);

#endif // FORMCOMPONENTSRESOLVER_HPP