#include "qtformdialogmanager.hpp"
#include "qtformdialog.hpp"

#include <QWidget>
#include <utility>

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
                                       QWidget* parent,
                                       std::function<void(const FormComponents&)> commit,
                                       Data* data)
{
    openOrFocus(formIdKey, QString(), components, nullptr, parent, std::move(commit), data);
}

void QtFormDialogManager::openOrFocus(const QString& formIdKey,
                                       const QString& recordType,
                                       FormComponents* components,
                                       void* recordPtr,
                                       QWidget* parent,
                                       std::function<void(const FormComponents&)> commit,
                                       Data* data)
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

    auto* dlg = new QtFormDialog(formIdKey, components, parent, std::move(commit), data);
    dlg->setModal(false);

    auto factoryIt = m_factories.find(recordType);
    if (factoryIt != m_factories.end() && factoryIt.value())
    {
        QWidget* customWidget = factoryIt.value()(dlg->workingComponents(), recordPtr, dlg);
        if (customWidget)
            dlg->setCustomWidget(customWidget);
    }

    m_dialogs.insert(formIdKey, dlg);
    connect(dlg, &QObject::destroyed, this, &QtFormDialogManager::onDialogDestroyed);
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

void QtFormDialogManager::openOrFocus(const QString& formIdKey,
                                       const QString& recordType,
                                       std::unique_ptr<RecordEditSession> session,
                                       QWidget* parent,
                                       Data* data)
{
    if (!session) return;
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

    FormComponents* components = session->workingComponents();
    auto* recordPtr = session->workingRecord();
    auto* dlg = new QtFormDialog(formIdKey, components, parent,
                                 std::function<void(const FormComponents&)>(),
                                 data, std::move(session));
    dlg->setModal(false);

    auto factoryIt = m_factories.find(recordType);
    if (factoryIt != m_factories.end() && factoryIt.value())
    {
        // The factory gets the working copy, so a widget that writes to
        // recordPtr edits the session rather than the base record.
        QWidget* customWidget = factoryIt.value()(dlg->workingComponents(), recordPtr, dlg);
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
