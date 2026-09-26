#ifndef BLANKRECORDFACTORY_H
#define BLANKRECORDFACTORY_H

#include <QString>

#include <memory>

class BaseCollection;
class BaseRecord;

/// Creates a blank record of a collection's type, ready for AddRecordCommand.
///
/// Dispatch is by dynamic_cast over the shared record-type list rather than a
/// hand-written switch, so a type becomes creatable the moment it is in that
/// list and cannot be forgotten here. The implementation lives in a .cpp
/// because the record-type list needs every record header in scope.
///
/// Game settings and global variables sit outside the component architecture
/// and are handled explicitly.
class BlankRecordFactory
{
public:
    /// True when a blank record of this collection's type can be created.
    static bool supports(BaseCollection* collection);

    /// Builds a blank record stamped with the given editor ID and FormID, or
    /// returns nullptr when the type is not creatable.
    static std::unique_ptr<BaseRecord> create(BaseCollection* collection,
        const QString& editorId, quint32 formId);
};

#endif // BLANKRECORDFACTORY_H
