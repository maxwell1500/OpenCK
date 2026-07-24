#include "qtformdialogmanager.hpp"
#include "qtformdialog.hpp"

#include <QWidget>

namespace openck {

QtFormDialogManager& QtFormDialogManager::instance()
{
    static QtFormDialogManager inst;
    return inst;
}

QtFormDialogManager::QtFormDialogManager(QObject* parent)
    : QObject(parent)
{
}

QtFormDialogManager::~QtFormDialogManager() = default;

void QtFormDialogManager::openOrFocus(const QString& formIdKey,
                                      FormComponents* components,
                                      QWidget* parent)
{
    if (!components) return;

    auto it = m_dialogs.find(formIdKey);
    if (it != m_dialogs.end() && it.value())
    {
        // Dialog already open for this form ID — just raise it.
        QtFormDialog* dlg = it.value();
        dlg->raise();
        dlg->activateWindow();
        dlg->show();
        dlg->setFocus();
        return;
    }

    auto* dlg = new QtFormDialog(formIdKey, components, parent);
    m_dialogs.insert(formIdKey, dlg);
    connect(dlg, &QObject::destroyed, this, &QtFormDialogManager::onDialogDestroyed);
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

void QtFormDialogManager::closeAll()
{
    // Take a copy of the values because destroying the dialogs
    // mutates m_dialogs via the destroyed() signal.
    const auto all = m_dialogs.values();
    for (QtFormDialog* dlg : all)
    {
        if (dlg) dlg->close();
    }
    m_dialogs.clear();
}

void QtFormDialogManager::onDialogDestroyed(QObject* obj)
{
    // Remove the destroyed dialog from the registry.
    for (auto it = m_dialogs.begin(); it != m_dialogs.end(); ++it)
    {
        if (it.value() == obj)
        {
            m_dialogs.erase(it);
            return;
        }
    }
}

} // namespace openck
