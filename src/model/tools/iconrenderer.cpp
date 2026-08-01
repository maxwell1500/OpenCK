#include "iconrenderer.hpp"

#include <QCoreApplication>
#include <QDir>

#include "libs/files/log/logger.hpp"

int IconRenderer::contextSize(Context ctx)
{
    switch (ctx)
    {
    case Context::Inventory:   return 128;
    case Context::Workshop:    return 512;
    case Context::ShipBuilder: return 512;
    }
    return 512;
}

QString IconRenderer::contextName(Context ctx)
{
    switch (ctx)
    {
    case Context::Inventory:   return QStringLiteral("Inventory");
    case Context::Workshop:    return QStringLiteral("Workshop");
    case Context::ShipBuilder: return QStringLiteral("Ship Builder");
    }
    return QStringLiteral("Unknown");
}

QVector<IconRenderer::Light> IconRenderer::defaultRig()
{
    QVector<Light> rig;
    rig.append({ QStringLiteral("FillWarm"),
                 QColor(255, 158, 89),   // warm orange fill
                 -3.2, -2.4, 1.6, 3.0 });
    rig.append({ QStringLiteral("RimCool"),
                 QColor(89, 140, 255),   // cool blue rim
                 3.2, 2.6, 1.2, 2.2 });
    rig.append({ QStringLiteral("KeyLight"),
                 QColor(255, 247, 235),  // near-white key
                 0.0, -4.2, 3.4, 4.5 });
    return rig;
}

QString IconRenderer::scriptPath()
{
    return QCoreApplication::applicationDirPath()
        + QStringLiteral("/scripts/blender/icon_generator.py");
}

QStringList IconRenderer::blenderArguments(const QString& blenderExecutable,
                                           const QString& nifPath,
                                           const QString& outputIconPath,
                                           Context ctx)
{
    QStringList args;
    args << blenderExecutable
         << QStringLiteral("--background")
         << QStringLiteral("--python")
         << scriptPath()
         << QStringLiteral("--")
         << nifPath
         << outputIconPath
         << QString::number(contextSize(ctx));
    return args;
}
