#ifndef QTFORMDIALOGMANAGER_HPP
#define QTFORMDIALOGMANAGER_HPP

// QtFormDialogManager — singleton that keeps a registry of open
// QtFormDialogs, keyed by form ID. Opening a dialog for a record
// whose form ID is already open just focuses the existing dialog
// rather than creating a duplicate. Mirrors the real CK's
// QtCreationKitFormDialogManager (see docs/CK_Real_Integration_Plan.md).

#include "component.hpp"
#include "formcomponents.hpp"

#include <QHash>
#include <QObject>
#include <QString>

class QWidget;

namespace openck {

class QtFormDialog;

class QtFormDialogManager : public QObject
{
    Q_OBJECT

public:
    static QtFormDialogManager& instance();

    // Open (or focus) a dialog for the given record. The formIdKey
    // is the deduplication key, typically the string form of the
    // record's form ID. The components pointer is non-owning and
    // must outlive the dialog (typically: it's a member of the
    // record itself). The parent widget is used as the dialog's
    // transient parent so the dialog stays on top of the main
    // window.
    void openOrFocus(const QString& formIdKey, FormComponents* components,
                     QWidget* parent = nullptr);

    // Closes all open dialogs. Used when a document is unloaded
    // so we don't keep dangling pointers to the old data tree.
    void closeAll();

    // The number of currently-open dialogs. Useful for tests.
    int openCount() const { return m_dialogs.size(); }

private slots:
    void onDialogDestroyed(QObject* obj);

private:
    explicit QtFormDialogManager(QObject* parent = nullptr);
    ~QtFormDialogManager() override;
    QtFormDialogManager(const QtFormDialogManager&) = delete;
    QtFormDialogManager& operator=(const QtFormDialogManager&) = delete;

    QHash<QString, QtFormDialog*> m_dialogs;
};

} // namespace openck

#endif // QTFORMDIALOGMANAGER_HPP
