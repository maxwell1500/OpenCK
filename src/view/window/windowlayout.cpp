#include "windowlayout.hpp"

#include <DockManager.h>

#include "../../libs/files/log/logger.hpp"

namespace {

/// Places @p dock in @p area, but only when the dock is not docked anywhere yet
/// (newly created, floating or closed). Re-adding a dock that already sits in an
/// area tears its dock area down and rebuilds it, which is both wasteful and a
/// source of teardown instability, so a dock with a home keeps it.
void placeIfUnplaced(ads::CDockManager* manager, const QString& name,
                     ads::DockWidgetArea area, bool visible)
{
    ads::CDockWidget* dock = manager->findDockWidget(name);
    if (!dock)
        return;

    if (!dock->dockAreaWidget())
        manager->addDockWidget(area, dock);
    if (visible)
        dock->toggleView(true);
}

} // namespace

void WindowLayout::applyDefaultLayout(QMainWindow* window)
{
    auto* manager = window->findChild<ads::CDockManager*>();
    if (!manager) return;

    // Object Window left; Cell View and Inspector right; Warnings, Object
    // Palette and Landscape Editor along the bottom. The Render Window is the
    // pinned central dock and is never moved.
    placeIfUnplaced(manager, QStringLiteral("Object Window"), ads::LeftDockWidgetArea, true);
    placeIfUnplaced(manager, QStringLiteral("Cell View"), ads::RightDockWidgetArea, true);
    placeIfUnplaced(manager, QStringLiteral("Inspector"), ads::RightDockWidgetArea, true);
    placeIfUnplaced(manager, QStringLiteral("Warnings"), ads::BottomDockWidgetArea, true);
    placeIfUnplaced(manager, QStringLiteral("Object Palette"), ads::BottomDockWidgetArea, true);
    placeIfUnplaced(manager, QStringLiteral("Landscape Editor"), ads::BottomDockWidgetArea, false);
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