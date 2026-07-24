#include "lootwrapper.hpp"

#include "esmreader.hpp"
#include "tes4.hpp"
#include "logger.hpp"
#include "filepaths.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QTemporaryFile>
#include <QTextStream>
#include <QStandardPaths>

LootWrapper::LootWrapper(QObject* parent)
    : QObject(parent)
    , m_lootPath()
    , m_checked(false)
{
}

LootWrapper::~LootWrapper()
{
}

QString LootWrapper::findLootPath()
{
    if (!m_lootPath.isEmpty() && QFile::exists(m_lootPath))
    {
        return m_lootPath;
    }

    QString configPath = FilePaths::configFilePath();
    QSettings conf(configPath, QSettings::IniFormat);
    conf.beginGroup("OpenCK");
    QString customPath = conf.value("LOOTPath", "").toString();
    conf.endGroup();

    if (!customPath.isEmpty() && QFile::exists(customPath))
    {
        m_lootPath = customPath;
        m_checked = true;
        LOG_INFO(QString("Found LOOT from settings: %1").arg(customPath));
        return customPath;
    }

    QStringList searchPaths;

    QString localAppData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (!localAppData.isEmpty())
    {
        searchPaths << localAppData + "/LOOT/LOOT.exe";
    }

    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!appData.isEmpty())
    {
        searchPaths << appData + "/LOOT/LOOT.exe";
    }

    searchPaths << "C:/Program Files/LOOT/LOOT.exe";
    searchPaths << "C:/Program Files (x86)/LOOT/LOOT.exe";

    QString programFiles = QString::fromLocal8Bit(qgetenv("ProgramFiles"));
    if (!programFiles.isEmpty())
    {
        searchPaths << programFiles + "/LOOT/LOOT.exe";
    }

    QString programFilesX86 = QString::fromLocal8Bit(qgetenv("ProgramFiles(x86)"));
    if (!programFilesX86.isEmpty())
    {
        searchPaths << programFilesX86 + "/LOOT/LOOT.exe";
    }

    QString steamPath = "C:/Program Files (x86)/Steam/steamapps/common";
    searchPaths << steamPath + "/LOOT/LOOT.exe";

    QString steamPathAlt = "C:/Program Files/Steam/steamapps/common";
    searchPaths << steamPathAlt + "/LOOT/LOOT.exe";

    for (const QString& path : searchPaths)
    {
        if (QFile::exists(path))
        {
            m_lootPath = path;
            m_checked = true;
            LOG_INFO(QString("Found LOOT at: %1").arg(path));
            return path;
        }
    }

    m_checked = true;
    LOG_WARNING("LOOT not found in any standard locations");
    return QString();
}

bool LootWrapper::isAvailable()
{
    if (!m_checked)
    {
        findLootPath();
    }
    return !m_lootPath.isEmpty();
}

void LootWrapper::setLootPath(const QString& path)
{
    m_lootPath = path;
    m_checked = true;
}

QString LootWrapper::getLootPath() const
{
    return m_lootPath;
}

QVector<QString> LootWrapper::sortPlugins(const QVector<QString>& plugins)
{
    if (plugins.isEmpty())
    {
        return plugins;
    }

    if (!isAvailable())
    {
        LOG_WARNING("LOOT not available, falling back to alphabetical sort");
        QVector<QString> sorted = plugins;
        std::sort(sorted.begin(), sorted.end(), [](const QString& a, const QString& b)
        {
            return a.toLower() < b.toLower();
        });
        return sorted;
    }

    QTemporaryFile tempFile(QDir::tempPath() + "/loot_plugins_XXXXXX.txt");
    if (!tempFile.open())
    {
        LOG_ERROR("Failed to create temporary file for LOOT sorting");
        return plugins;
    }

    QTextStream stream(&tempFile);
    for (const QString& plugin : plugins)
    {
        stream << plugin << "\n";
    }
    tempFile.close();

    QString inputFile = tempFile.fileName();
    QString outputFile = inputFile + ".sorted";

    QProcess process;
    process.setWorkingDirectory(QFileInfo(m_lootPath).path());
    process.start(m_lootPath, {"--sort", "--plugin-list", inputFile});

    if (!process.waitForFinished(30000))
    {
        LOG_ERROR("LOOT process timed out");
        process.kill();
        QFile::remove(inputFile);
        return plugins;
    }

    if (process.exitCode() != 0)
    {
        LOG_ERROR(QString("LOOT returned error: %1").arg(process.readAllStandardError()));
        QFile::remove(inputFile);
        QFile::remove(outputFile);

        QVector<QString> sorted = plugins;
        std::sort(sorted.begin(), sorted.end(), [](const QString& a, const QString& b)
        {
            return a.toLower() < b.toLower();
        });
        return sorted;
    }

    QVector<QString> sortedPlugins;
    QFile outFile(outputFile);
    if (outFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&outFile);
        while (!in.atEnd())
        {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty() && plugins.contains(line))
            {
                sortedPlugins.append(line);
            }
        }
        outFile.close();
    }

    QFile::remove(inputFile);
    QFile::remove(outputFile);

    if (sortedPlugins.size() != plugins.size())
    {
        LOG_WARNING("LOOT returned unexpected number of plugins, falling back to input order");
        return plugins;
    }

    LOG_INFO(QString("LOOT sorted %1 plugins successfully").arg(sortedPlugins.size()));
    return sortedPlugins;
}

QStringList LootWrapper::getMasterPlugins(const QString& pluginPath)
{
    QStringList masters;

    QFile file(pluginPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_ERROR(QString("Failed to open plugin file: %1").arg(pluginPath));
        return masters;
    }

    quint32 recordType;
    if (file.read(reinterpret_cast<char*>(&recordType), sizeof(quint32)) != sizeof(quint32))
    {
        file.close();
        return masters;
    }

    if (recordType != 'TES4')
    {
        file.close();
        return masters;
    }

    quint32 recordSize;
    if (file.read(reinterpret_cast<char*>(&recordSize), sizeof(quint32)) != sizeof(quint32))
    {
        file.close();
        return masters;
    }

    Flags flags;
    if (file.read(reinterpret_cast<char*>(&flags.val), sizeof(quint32)) != sizeof(quint32))
    {
        file.close();
        return masters;
    }

    quint32 recordId;
    if (file.read(reinterpret_cast<char*>(&recordId), sizeof(quint32)) != sizeof(quint32))
    {
        file.close();
        return masters;
    }

    quint8 vcData[8];
    if (file.read(reinterpret_cast<char*>(vcData), 8) != 8)
    {
        file.close();
        return masters;
    }

    quint16 version;
    if (file.read(reinterpret_cast<char*>(&version), sizeof(quint16)) != sizeof(quint16))
    {
        file.close();
        return masters;
    }

    quint16 unknown;
    if (file.read(reinterpret_cast<char*>(&unknown), sizeof(quint16)) != sizeof(quint16))
    {
        file.close();
        return masters;
    }

    qint64 recordEnd = file.pos() + recordSize;

    while (file.pos() < recordEnd)
    {
        quint32 subName;
        if (file.read(reinterpret_cast<char*>(&subName), sizeof(quint32)) != sizeof(quint32))
        {
            break;
        }

        quint32 subSize;
        if (file.read(reinterpret_cast<char*>(&subSize), sizeof(quint32)) != sizeof(quint32))
        {
            break;
        }

        if (subName == 'MAST')
        {
            QByteArray nameData = file.read(subSize);
            QString masterName = QString::fromLatin1(nameData);
            if (masterName.endsWith(QLatin1Char('\0')))
            {
                masterName.chop(1);
            }
            masters.append(masterName);

            quint32 dataSubName;
            if (file.read(reinterpret_cast<char*>(&dataSubName), sizeof(quint32)) != sizeof(quint32))
            {
                break;
            }

            quint64 dataSize;
            if (file.read(reinterpret_cast<char*>(&dataSize), sizeof(quint64)) != sizeof(quint64))
            {
                break;
            }
        }
        else
        {
            if (subSize > 0)
            {
                file.seek(file.pos() + subSize);
            }
        }
    }

    file.close();
    LOG_INFO(QString("Found %1 masters in %2").arg(masters.size()).arg(pluginPath));
    return masters;
}
