#include "windowlayout.hpp"

#include <QDockWidget>
#include <QTabWidget>

#include "../../libs/files/log/logger.hpp"

void WindowLayout::applyDefaultLayout(QMainWindow* window)
{
    QList<QDockWidget*> docks = window->findChildren<QDockWidget*>();
    for (QDockWidget* dock : docks)
    {
        window->removeDockWidget(dock);
    }

    QDockWidget* objectWindowDock = nullptr;
    QDockWidget* viewportDock = nullptr;
    QDockWidget* propertiesDock = nullptr;
    QDockWidget* paletteDock = nullptr;
    QDockWidget* cellDock = nullptr;
    QDockWidget* landscapeDock = nullptr;
    QList<QDockWidget*> others;

    for (QDockWidget* dock : docks)
    {
        QString title = dock->windowTitle();
        if (title.contains("Object Window", Qt::CaseInsensitive) ||
            title.contains("Object", Qt::CaseInsensitive))
            objectWindowDock = dock;
        else if (title.contains("Viewport", Qt::CaseInsensitive) ||
                 title.contains("3D", Qt::CaseInsensitive))
            viewportDock = dock;
        else if (title.contains("Properties", Qt::CaseInsensitive) ||
                 title.contains("Inspector", Qt::CaseInsensitive) ||
                 title.contains("Form", Qt::CaseInsensitive))
            propertiesDock = dock;
        else if (title.contains("Palette", Qt::CaseInsensitive) ||
                 title.contains("Object Palette", Qt::CaseInsensitive))
            paletteDock = dock;
        else if (title.contains("Cell", Qt::CaseInsensitive))
            cellDock = dock;
        else if (title.contains("Landscape", Qt::CaseInsensitive))
            landscapeDock = dock;
        else
            others.append(dock);
    }

    if (objectWindowDock)
        window->addDockWidget(Qt::LeftDockWidgetArea, objectWindowDock);

    if (viewportDock)
        window->addDockWidget(Qt::RightDockWidgetArea, viewportDock);

    if (propertiesDock)
    {
        window->addDockWidget(Qt::RightDockWidgetArea, propertiesDock);
        if (viewportDock)
            window->splitDockWidget(viewportDock, propertiesDock, Qt::Vertical);
    }

    if (cellDock)
    {
        window->addDockWidget(Qt::RightDockWidgetArea, cellDock);
        if (propertiesDock)
            window->tabifyDockWidget(propertiesDock, cellDock);
        else if (viewportDock)
            window->tabifyDockWidget(viewportDock, cellDock);
    }

    if (paletteDock)
        window->addDockWidget(Qt::BottomDockWidgetArea, paletteDock);

    if (landscapeDock)
        window->addDockWidget(Qt::BottomDockWidgetArea, landscapeDock);

    for (QDockWidget* dock : others)
        window->addDockWidget(Qt::BottomDockWidgetArea, dock);

    if (viewportDock)
        viewportDock->raise();
}

void WindowLayout::saveLayout(QMainWindow* window, QSettings& settings)
{
    settings.setValue("dockState", window->saveState());
}

void WindowLayout::restoreLayout(QMainWindow* window, QSettings& settings)
{
    QByteArray state = settings.value("dockState").toByteArray();
    if (!state.isEmpty())
    {
        if (!window->restoreState(state))
        {
            LOG_WARNING("Failed to restore dock layout; applying default");
            applyDefaultLayout(window);
        }
    }
    else
    {
        applyDefaultLayout(window);
    }
}
