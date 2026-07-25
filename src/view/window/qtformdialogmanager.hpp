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

#include <QHash>
#include <QObject>
#include <QString>

#include <functional>

class QWidget;

namespace openck {

class QtFormDialog;

// A factory function that creates a custom data widget for a record
// type. The widget will be placed below the generic component grid
// in the QtFormDialog. The factory receives the record's components,
// an opaque pointer to the record struct itself (cast by the widget),
// and the parent widget for the dialog.
using FormDataWidgetFactory = std::function<QWidget*(FormComponents*, void* recordPtr, QWidget*)>;

class QtFormDialogManager : public QObject
{
    Q_OBJECT

public:
    static QtFormDialogManager& instance();

    // Open (or focus) a dialog for the given record. The formIdKey
    // is the deduplication key, typically the string form of the
    // record's form ID. The components pointer is non-owning and
    // must outlive the dialog.
    void openOrFocus(const QString& formIdKey, FormComponents* components,
                     QWidget* parent = nullptr);

    // Overload that accepts a record type string and optional record
    // pointer. If a factory is registered for that type, the dialog
    // will include a custom widget below the generic component grid.
    void openOrFocus(const QString& formIdKey, const QString& recordType,
                     FormComponents* components, void* recordPtr = nullptr,
                     QWidget* parent = nullptr);

    // Register a factory that creates custom data widgets for a
    // specific record type (e.g. "NPC_", "RACE", "CELL").
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
