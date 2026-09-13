#include "modmanagerdetection.hpp"
#include "logger.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>

ModManagerDetection::ModManagerInfo ModManagerDetection::detect()
{
    ModManagerInfo mo2 = detectMO2();
    if (mo2.type != ModManager::None)
    {
        return mo2;
    }

    ModManagerInfo vortex = detectVortex();
    if (vortex.type != ModManager::None)
    {
        return vortex;
    }

    ModManagerInfo empty;
    empty.type = ModManager::None;
    return empty;
}

ModManagerDetection::ModManagerInfo ModManagerDetection::detectMO2()
{
    ModManagerInfo info;
    info.type = ModManager::MO2;

    QSettings settings;
    QString customPath = settings.value("Paths/MO2").toString();
    if (!customPath.isEmpty())
    {
        QFileInfo customExe(customPath + "/ModOrganizer.exe");
        if (customExe.exists())
        {
            info.installPath = customPath;
            LOG_INFO(QString("ModOrganizer2 found at configured path: %1").arg(info.installPath));
        }
    }

    if (info.installPath.isEmpty())
    {
        QStringList searchPaths;

        QString localAppData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        searchPaths << localAppData + "/ModOrganizer2"
                    << localAppData + "/../Local/ModOrganizer2"
                    << QDir::homePath() + "/AppData/Local/ModOrganizer2";

        QString programFiles = QProcessEnvironment::systemEnvironment().value("ProgramFiles", "C:/Program Files");
        QString programFilesX86 = QProcessEnvironment::systemEnvironment().value("ProgramFiles(x86)", "C:/Program Files (x86)");
        searchPaths << programFiles + "/ModOrganizer2"
                    << programFilesX86 + "/ModOrganizer2";

        QSettings regSettings("HKEY_CURRENT_USER\\Software\\ModOrganizer2", QSettings::NativeFormat);
        QString regPath = regSettings.value("CurrentInstance").toString();
        if (!regPath.isEmpty())
        {
            QFileInfo regInfo(regPath);
            if (regInfo.exists())
            {
                info.installPath = regPath;
            }
        }

        if (info.installPath.isEmpty())
        {
            for (const QString& path : searchPaths)
            {
                QFileInfo exeInfo(path + "/ModOrganizer.exe");
                if (exeInfo.exists())
                {
                    info.installPath = path;
                    break;
                }
            }
        }
    }

    if (info.installPath.isEmpty())
    {
        info.type = ModManager::None;
        return info;
    }

    LOG_INFO(QString("ModOrganizer2 found at: %1").arg(info.installPath));

    QFileInfo exeInfo(info.installPath + "/ModOrganizer.exe");
    if (exeInfo.exists())
    {
        info.version = exeInfo.lastModified().toString("yyyy.MM.dd");
    }

    QString iniPath = info.installPath + "/ModOrganizer.ini";
    parseMo2Ini(iniPath, info);

    info.isRunning = isMO2Running();
    return info;
}

ModManagerDetection::ModManagerInfo ModManagerDetection::detectVortex()
{
    ModManagerInfo info;
    info.type = ModManager::Vortex;

    QSettings settings;
    QString customPath = settings.value("Paths/Vortex").toString();
    if (!customPath.isEmpty())
    {
        QFileInfo customExe(customPath + "/Vortex.exe");
        if (customExe.exists())
        {
            info.installPath = customPath;
            LOG_INFO(QString("Vortex found at configured path: %1").arg(info.installPath));
        }
    }

    if (info.installPath.isEmpty())
    {
        QStringList searchPaths;

        QString localAppData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        searchPaths << localAppData + "/Vortex"
                    << localAppData + "/../Local/Vortex"
                    << QDir::homePath() + "/AppData/Local/Vortex";

        QString appData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        searchPaths << appData + "/../Roaming/Vortex";

        QString programFiles = QProcessEnvironment::systemEnvironment().value("ProgramFiles", "C:/Program Files");
        QString programFilesX86 = QProcessEnvironment::systemEnvironment().value("ProgramFiles(x86)", "C:/Program Files (x86)");
        searchPaths << programFiles + "/Vortex"
                    << programFilesX86 + "/Vortex";

        for (const QString& path : searchPaths)
        {
            QFileInfo exeInfo(path + "/Vortex.exe");
            if (exeInfo.exists())
            {
                info.installPath = path;
                break;
            }
        }
    }

    if (info.installPath.isEmpty())
    {
        info.type = ModManager::None;
        return info;
    }

    LOG_INFO(QString("Vortex found at: %1").arg(info.installPath));

    QFileInfo exeInfo(info.installPath + "/Vortex.exe");
    if (exeInfo.exists())
    {
        info.version = exeInfo.lastModified().toString("yyyy.MM.dd");
    }

    QString customConfig = settings.value("Paths/VortexConfig").toString();
    QString configPath = customConfig.isEmpty()
        ? QDir::homePath() + "/AppData/Roaming/Vortex/vortex.json"
        : customConfig;
    QFile configFile(configPath);
    if (configFile.exists() && configFile.open(QIODevice::ReadOnly))
    {
        QByteArray data = configFile.readAll();
        configFile.close();
        info.gamePath = parseVortexField(QString::fromUtf8(data), QStringLiteral("gamePath"));
    }

    info.isRunning = isVortexRunning();
    return info;
}

bool ModManagerDetection::isMO2Running()
{
    QProcess process;
    process.start("tasklist", QStringList() << "/FI" << "IMAGENAME eq ModOrganizer.exe" << "/NH");
    process.waitForFinished(5000);
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    return output.contains("ModOrganizer.exe", Qt::CaseInsensitive);
}

bool ModManagerDetection::isVortexRunning()
{
    QProcess process;
    process.start("tasklist", QStringList() << "/FI" << "IMAGENAME eq Vortex.exe" << "/NH");
    process.waitForFinished(5000);
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    return output.contains("Vortex.exe", Qt::CaseInsensitive);
}

QString ModManagerDetection::getMO2Profile()
{
    ModManagerInfo info = detectMO2();
    if (info.type == ModManager::None)
    {
        return QString();
    }
    return info.selectedProfile;
}

QString ModManagerDetection::getVortexProfile()
{
    ModManagerInfo info = detectVortex();
    if (info.type == ModManager::None)
    {
        return QString();
    }

    QSettings settings;
    QString customConfig = settings.value("Paths/VortexConfig").toString();
    QString configPath = customConfig.isEmpty()
        ? QDir::homePath() + "/AppData/Roaming/Vortex/vortex.json"
        : customConfig;
    QFile configFile(configPath);
    if (!configFile.exists() || !configFile.open(QIODevice::ReadOnly))
    {
        return QString();
    }

    QByteArray data = configFile.readAll();
    configFile.close();

    return parseVortexField(QString::fromUtf8(data), QStringLiteral("activeProfile"));
}

QStringList ModManagerDetection::getInstalledMods(ModManager manager)
{
    QStringList mods;

    if (manager == ModManager::MO2)
    {
        ModManagerInfo info = detectMO2();
        if (info.type == ModManager::None)
        {
            return mods;
        }

        QString modsPath = info.modsDirectory.isEmpty()
            ? info.installPath + "/mods"
            : info.modsDirectory;

        QDir modsDir(modsPath);
        if (modsDir.exists())
        {
            QFileInfoList entries = modsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QFileInfo& entry : entries)
            {
                mods.append(entry.fileName());
            }
        }
    }
    else if (manager == ModManager::Vortex)
    {
        ModManagerInfo info = detectVortex();
        if (info.type == ModManager::None)
        {
            return mods;
        }

        QSettings settings;
        QString customMods = settings.value("Paths/VortexMods").toString();
        QString modsPath = customMods.isEmpty()
            ? QDir::homePath() + "/AppData/Local/Vortex/mods"
            : customMods;
        QDir modsDir(modsPath);
        if (modsDir.exists())
        {
            QFileInfoList entries = modsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QFileInfo& entry : entries)
            {
                mods.append(entry.fileName());
            }
        }
    }

    return mods;
}

bool ModManagerDetection::parseMo2Ini(const QString& iniPath, ModManagerInfo& out)
{
    QFile iniFile(iniPath);
    if (!iniFile.exists() || !iniFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }

    QTextStream stream(&iniFile);
    bool inProfiles = false;
    while (!stream.atEnd())
    {
        QString line = stream.readLine().trimmed();
        if (line.startsWith("[") && line.endsWith("]"))
        {
            inProfiles = (line.mid(1, line.length() - 2) == "Profiles");
            continue;
        }
        if (line.isEmpty() || line.startsWith(";") || line.startsWith("#"))
        {
            continue;
        }
        if (!line.contains("="))
        {
            continue;
        }
        const QString key = line.section("=", 0, 0).trimmed();
        const QString value = line.section("=", 1).trimmed();
        if (inProfiles)
        {
            if (key == "profileName" && !value.isEmpty())
            {
                out.profiles.append(value);
            }
            else if (key == "selectedProfile" && !value.isEmpty())
            {
                out.selectedProfile = value;
            }
        }
        else
        {
            if (key.compare("gamePath", Qt::CaseInsensitive) == 0)
            {
                out.gamePath = QDir::toNativeSeparators(value);
            }
            else if (key.compare("modsDirectory", Qt::CaseInsensitive) == 0)
            {
                out.modsDirectory = QDir::toNativeSeparators(value);
            }
        }
    }
    iniFile.close();
    return true;
}

QString ModManagerDetection::parseVortexField(const QString& jsonContent, const QString& key)
{
    const int idx = jsonContent.indexOf("\"" + key + "\"");
    if (idx < 0) return QString();
    const int colonIdx = jsonContent.indexOf(":", idx);
    if (colonIdx < 0) return QString();
    const int quoteStart = jsonContent.indexOf("\"", colonIdx + 1);
    if (quoteStart < 0) return QString();
    const int quoteEnd = jsonContent.indexOf("\"", quoteStart + 1);
    if (quoteEnd < 0) return QString();
    return jsonContent.mid(quoteStart + 1, quoteEnd - quoteStart - 1);
}
