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

void QtFormDialogManager::registerFactory(const QString& recordType,
                                           FormDataWidgetFactory factory)
{
    if (factory)
        m_factories[recordType] = std::move(factory);
}

bool QtFormDialogManager::hasFactory(const QString& recordType) const
{
    return m_factories.contains(recordType);
}

void QtFormDialogManager::openOrFocus(const QString& formIdKey,
                                       FormComponents* components,
                                       QWidget* parent)
{
    openOrFocus(formIdKey, QString(), components, nullptr, parent);
}

void QtFormDialogManager::openOrFocus(const QString& formIdKey,
                                       const QString& recordType,
                                       FormComponents* components,
                                       void* recordPtr,
                                       QWidget* parent)
{
    if (!components) return;

    auto it = m_dialogs.find(formIdKey);
    if (it != m_dialogs.end() && it.value())
    {
        QtFormDialog* dlg = it.value();
        dlg->raise();
        dlg->activateWindow();
        dlg->show();
        dlg->setFocus();
        return;
    }

    auto* dlg = new QtFormDialog(formIdKey, components, parent);

    auto factoryIt = m_factories.find(recordType);
    if (factoryIt != m_factories.end() && factoryIt.value())
    {
        QWidget* customWidget = factoryIt.value()(components, recordPtr, dlg);
        if (customWidget)
            dlg->setCustomWidget(customWidget);
    }

    m_dialogs.insert(formIdKey, dlg);
    connect(dlg, &QObject::destroyed, this, &QtFormDialogManager::onDialogDestroyed);
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

void QtFormDialogManager::closeAll()
{
    const auto all = m_dialogs.values();
    for (QtFormDialog* dlg : all)
    {
        if (dlg) dlg->close();
    }
    m_dialogs.clear();
}

void QtFormDialogManager::onDialogDestroyed(QObject* obj)
{
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
