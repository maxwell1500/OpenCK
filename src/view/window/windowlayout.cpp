#include "windowlayout.hpp"

#include <DockManager.h>

#include "../../libs/files/log/logger.hpp"

void WindowLayout::applyDefaultLayout(QMainWindow* window)
{
    auto* manager = window->findChild<ads::CDockManager*>();
    if (manager)
    {
        for (auto* dock : manager->dockWidgetsMap())
        {
            dock->toggleView(true);
        }
    }
}

void WindowLayout::saveLayout(QMainWindow* window, QSettings& settings)
{
    auto* manager = window->findChild<ads::CDockManager*>();
    if (manager)
    {
        settings.setValue("adsDockState", manager->saveState());
    }
}

void WindowLayout::restoreLayout(QMainWindow* window, QSettings& settings)
{
    auto* manager = window->findChild<ads::CDockManager*>();
    if (manager)
    {
        QByteArray state = settings.value("adsDockState").toByteArray();
        if (!state.isEmpty())
        {
            if (!manager->restoreState(state))
            {
                LOG_WARNING("Failed to restore ADS dock layout; applying default");
                applyDefaultLayout(window);
            }
        }
        else
        {
            applyDefaultLayout(window);
        }
    }
    else
    {
        applyDefaultLayout(window);
    }
}