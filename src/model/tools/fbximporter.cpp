#include "fbximporter.hpp"

#include <QCoreApplication>
#include <QDir>

#include "libs/files/log/logger.hpp"

QString FbxImporter::exportScriptPath()
{
    return QCoreApplication::applicationDirPath()
        + QStringLiteral("/scripts/blender/nif_export.py");
}

QStringList FbxImporter::blenderArguments(const QString& blenderExecutable,
                                          const QString& fbxPath,
                                          const QString& nifPath,
                                          const Settings& settings)
{
    const QString script = settings.scriptPath.isEmpty()
        ? exportScriptPath() : settings.scriptPath;

    QStringList args;
    args << blenderExecutable
         << QStringLiteral("--background")
         << QStringLiteral("--python")
         << script
         << QStringLiteral("--")
         << fbxPath
         << nifPath
         << settings.game;
    return args;
}

bool FbxImporter::hasCustomProcessing(const Settings& settings)
{
    return !settings.weldSkin || !settings.keepBones
        || !settings.keepEditorMarkers || settings.physicsLod > 0;
}

QString FbxImporter::summary(const Settings& settings)
{
    QStringList parts;
    parts << QStringLiteral("weld skin: %1").arg(settings.weldSkin ? QStringLiteral("on") : QStringLiteral("off"));
    parts << QStringLiteral("bones: %1").arg(settings.keepBones ? QStringLiteral("keep") : QStringLiteral("strip"));
    parts << QStringLiteral("editor markers: %1").arg(settings.keepEditorMarkers ? QStringLiteral("keep") : QStringLiteral("strip"));
    if (settings.physicsLod > 0)
        parts << QStringLiteral("physics LOD: LOD%1").arg(settings.physicsLod);
    return parts.join(QStringLiteral(", "));
}
