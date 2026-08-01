#include "audiopipelinetools.hpp"

#include <QDir>
#include <QFileInfo>

namespace {

// Locates a file under the given tools directory using the known relative
// path. Returns empty when missing.
QString findUnder(const QString& toolsDir, const QStringList& relPaths)
{
    for (const QString& rel : relPaths)
    {
        const QString candidate = QDir(toolsDir).filePath(rel);
        if (QFileInfo::exists(candidate))
            return candidate;
    }
    return QString();
}

} // namespace

QString AudioPipelineTools::toolName(Tool tool)
{
    switch (tool)
    {
    case Tool::LipGenerator: return QStringLiteral("LipGenerator");
    case Tool::FaceFx: return QStringLiteral("FaceFX");
    case Tool::Wwise: return QStringLiteral("Wwise");
    case Tool::RoboVoicer: return QStringLiteral("RoboVoicer");
    }
    return QStringLiteral("Unknown");
}

QString AudioPipelineTools::findTool(Tool tool, const QString& toolsDir)
{
    switch (tool)
    {
    case Tool::LipGenerator:
        return findUnder(toolsDir, {
            QStringLiteral("LipGenerator/LipGenerator.exe"),
            QStringLiteral("LipGenerator/LipGenerator/LipGenerator.exe"),
        });
    case Tool::FaceFx:
        return findUnder(toolsDir, {
            QStringLiteral("FaceFX/ffxc.exe"),
            QStringLiteral("FaceFX/ffxc/ffxc.exe"),
        });
    case Tool::Wwise:
        return findUnder(toolsDir, {
            QStringLiteral("Wwise/Wwise.exe"),
            QStringLiteral("Wwise/Authoring/x64/Release/bin/Wwise.exe"),
        });
    case Tool::RoboVoicer:
        return findUnder(toolsDir, {
            QStringLiteral("RoboVoicer/RoboVoicer.exe"),
        });
    }
    return QString();
}

QString AudioPipelineTools::lipDataFile(const QString& toolsDir)
{
    return findUnder(toolsDir, {
        QStringLiteral("LipGenerator/FonixData.cdf"),
        QStringLiteral("LipGenerator/LipGenerator/FonixData.cdf"),
    });
}

QStringList AudioPipelineTools::lipGeneratorArguments(
    const QString& lipGeneratorPath, const QString& wavPath,
    const QString& lipPath, const QString& dataFile, int sampleRate)
{
    QStringList args;
    args << lipGeneratorPath
         << QStringLiteral("-wav") << wavPath
         << QStringLiteral("-out") << lipPath
         << QStringLiteral("-data") << dataFile
         << QStringLiteral("-rate") << QString::number(sampleRate);
    return args;
}

QStringList AudioPipelineTools::facefxArguments(const QString& ffxcPath,
                                                const QString& actorProject,
                                                const QString& outputDir)
{
    QStringList args;
    args << ffxcPath
         << QStringLiteral("-project") << actorProject
         << QStringLiteral("-out") << outputDir;
    return args;
}

QStringList AudioPipelineTools::roboVoicerArguments(const QString& roboVoicerPath,
                                                    const QString& text,
                                                    const QString& outputWavPath)
{
    QStringList args;
    args << roboVoicerPath
         << QStringLiteral("-text") << text
         << QStringLiteral("-out") << outputWavPath;
    return args;
}

int AudioPipelineTools::wwiseExternalCodecId()
{
    return 4;  // the Creation Kit's [Wwise] iDefaultExternalCodecID value
}
