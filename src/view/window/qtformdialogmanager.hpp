#ifndef QTFORMDIALOGMANAGER_HPP
#define QTFORMDIALOGMANAGER_HPP

// QtFormDialogManager — singleton that keeps a registry of open
// QtFormDialogs, keyed by form ID. Opening a dialog for a record
// whose form ID is already open just focuses the existing dialog
// rather than creating a duplicate. Mirrors the real CK's
// QtCreationKitFormDialogManager (see docs/CK_Real_Integration_Plan.md).
//
// Complex record types (NPC, RACE, CELL, etc.) can register a
// FormDataWidgetFactory to create a record-specific widget that
// appears below the generic component grid in the dialog.

#include "../libs/components/component.hpp"
#include "../libs/components/formcomponents.hpp"
#include "recordeditsession.hpp"

#include <QHash>
#include <QObject>
#include <QString>

#include <functional>
#include <memory>

class QWidget;
class Data;

namespace openck {

class QtFormDialog;

// A factory function that creates a custom data widget for a record
// type. The widget will be placed below the generic component grid
// in the QtFormDialog. The factory receives the record's components,
// an opaque pointer to the record struct itself (cast by the widget),
// and the parent widget for the dialog.
//
// The record pointer is the session's working copy, never the live record, so
// a widget may edit it freely; the dialog discards it on Cancel and commits it
// through the undo stack on OK.
using FormDataWidgetFactory = std::function<QWidget*(FormComponents*, void* recordPtr, QWidget*)>;

/// Singleton registry of open QtFormDialogs, deduplicating by form ID.
class QtFormDialogManager : public QObject
{
    Q_OBJECT

public:
    static QtFormDialogManager& instance();

    /// Opens a dialog for the record, or focuses it if already open.
    void openOrFocus(const QString& formIdKey, FormComponents* components,
                     QWidget* parent = nullptr,
                     std::function<void(const FormComponents&)> commit = {},
                     Data* data = nullptr);

    // Overload that accepts a record type string and optional record
    // pointer. If a factory is registered for that type, the dialog
    // will include a custom widget below the generic component grid.
    //
    // Legacy: recordPtr is the LIVE record, so a custom widget that writes to
    // it mutates the base record with no undo entry. Prefer the session
    // overload below. Kept for dialogs with no record to edit.
    void openOrFocus(const QString& formIdKey, const QString& recordType,
                     FormComponents* components, void* recordPtr = nullptr,
                     QWidget* parent = nullptr,
                     std::function<void(const FormComponents&)> commit = {},
                     Data* data = nullptr);

    /// Preferred overload: the caller builds a typed RecordEditSession, and
    /// the dialog takes its working record and components from it. Ownership
    /// passes to the dialog, which commits on OK and discards on Cancel.
    void openOrFocus(const QString& formIdKey, const QString& recordType,
                     std::unique_ptr<RecordEditSession> session,
                     QWidget* parent = nullptr,
                     Data* data = nullptr);

    /// Registers a factory that builds a custom data widget for a record type.
    void registerFactory(const QString& recordType,
                         FormDataWidgetFactory factory);

    bool hasFactory(const QString& recordType) const;

    void closeAll();
    int openCount() const { return m_dialogs.size(); }

private slots:
    void onDialogDestroyed(QObject* obj);

private:
    explicit QtFormDialogManager(QObject* parent = nullptr);
    ~QtFormDialogManager() override;
    QtFormDialogManager(const QtFormDialogManager&) = delete;
    QtFormDialogManager& operator=(const QtFormDialogManager&) = delete;

    QHash<QString, QtFormDialog*> m_dialogs;
    QHash<QString, FormDataWidgetFactory> m_factories;
};

} // namespace openck

#endif // QTFORMDIALOGMANAGER_HPP
