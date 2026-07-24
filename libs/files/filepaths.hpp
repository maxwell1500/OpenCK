#ifndef FILEPATH_H
#define FILEPATH_H

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <QMap>
#include <QMetaType>
#include <QPair>
#include <QVector>
#include <QSet>

#ifdef _WIN32
#include <Windows.h>
#endif

enum GameId
{
    Game_None = 0,
    Game_Morrowind,
    Game_Oblivion,
    Game_Skyrim,
    Game_SkyrimSpecialEdition,
    Game_SkyrimAnniversaryEdition,
    Game_Fallout3,
    Game_FalloutNewVegas,
    Game_Fallout4,
    Game_Starfield,
    Game_NumGames
};

Q_DECLARE_METATYPE(GameId)

inline QString getEnvironmentVariable(const char* name)
{
#ifdef _WIN32
    char* buffer = nullptr;
    size_t size = 0;
    if (_dupenv_s(&buffer, &size, name) == 0 && buffer != nullptr)
    {
        QString result = QString::fromUtf8(buffer);
        free(buffer);
        return result;
    }
    return QString();
#else
    const char* value = getenv(name);
    return value ? QString::fromUtf8(value) : QString();
#endif
}

struct FilePaths
{
    QDir appDir;
    QDir dataDir;
    QDir programDir;

    QString appPath;
    QString configPath;
    QString iniName = "editor.ini";
    GameId gameId = Game_None;

    FilePaths()
    {
    }

    FilePaths(QString applicationName)
    {
        init(applicationName, Game_None);
    }

    FilePaths(QString applicationName, GameId game)
    {
        init(applicationName, game);
    }

    void init(QString applicationName, GameId game)
    {
        gameId = game;
        appDir = QCoreApplication::applicationDirPath();
        configPath = appDir.path() + "/" + iniName;

        dataDir = getDefaultDataDir(game);

        QString homeDir = getEnvironmentVariable("HOME");
        QString programFiles = getEnvironmentVariable("PROGRAMFILES(X86)");
        if (programFiles.isEmpty())
            programFiles = "C:/Program Files (x86)";

        QMap<GameId, QString> defaultPaths;
        defaultPaths.insert(Game_Morrowind, QString("%1/.openmw/data").arg(homeDir));
        defaultPaths.insert(Game_Oblivion, QString("%1/Steam/steamapps/common/Oblivion").arg(programFiles));
        defaultPaths.insert(Game_Skyrim, QString("%1/Steam/steamapps/common/Skyrim").arg(programFiles));
        defaultPaths.insert(Game_SkyrimSpecialEdition, QString("%1/Steam/steamapps/common/Skyrim Special Edition").arg(programFiles));
        defaultPaths.insert(Game_SkyrimAnniversaryEdition, QString("%1/Steam/steamapps/common/Skyrim Special Edition").arg(programFiles));
        defaultPaths.insert(Game_Fallout3, QString("%1/Steam/steamapps/common/Fallout 3 GOTY").arg(programFiles));
        defaultPaths.insert(Game_FalloutNewVegas, QString("%1/Steam/steamapps/common/Fallout New Vegas").arg(programFiles));
        defaultPaths.insert(Game_Fallout4, QString("%1/Steam/steamapps/common/Fallout 4").arg(programFiles));
        defaultPaths.insert(Game_Starfield, QString("%1/Steam/steamapps/common/Starfield").arg(programFiles));

        if (defaultPaths.contains(game) && !defaultPaths[game].isEmpty())
        {
            dataDir.setPath(defaultPaths[game]);
        }

        if (gameId == Game_None)
        {
            auto detected = detectGames();
            if (!detected.isEmpty())
            {
                dataDir = QDir(detected.first().dataPath);
            }
            else
            {
                dataDir.setPath(QString("%1/Steam/steamapps/common/Skyrim").arg(programFiles));
            }
        }
    }

    static QDir getDefaultDataDir(GameId game)
    {
        QString homeDir = getEnvironmentVariable("HOME");
        QString programFiles = getEnvironmentVariable("PROGRAMFILES(X86)");
        if (programFiles.isEmpty())
            programFiles = "C:/Program Files (x86)";

        if (game == Game_None)
        {
            return QDir(QString("%1/Steam/steamapps/common/Skyrim/Data").arg(programFiles));
        }

        QMap<GameId, QString> dataPaths;
        dataPaths.insert(Game_Morrowind, QString("%1/.openmw/data").arg(homeDir));
        dataPaths.insert(Game_Oblivion, QString("%1/Steam/steamapps/common/Oblivion/Data").arg(programFiles));
        dataPaths.insert(Game_Skyrim, QString("%1/Steam/steamapps/common/Skyrim/Data").arg(programFiles));
        dataPaths.insert(Game_SkyrimSpecialEdition, QString("%1/Steam/steamapps/common/Skyrim Special Edition/Data").arg(programFiles));
        dataPaths.insert(Game_SkyrimAnniversaryEdition, QString("%1/Steam/steamapps/common/Skyrim Special Edition/Data").arg(programFiles));
        dataPaths.insert(Game_Fallout3, QString("%1/Steam/steamapps/common/Fallout 3 GOTY/Data").arg(programFiles));
        dataPaths.insert(Game_FalloutNewVegas, QString("%1/Steam/steamapps/common/Fallout New Vegas/Data").arg(programFiles));
        dataPaths.insert(Game_Fallout4, QString("%1/Steam/steamapps/common/Fallout 4/Data").arg(programFiles));
        dataPaths.insert(Game_Starfield, QString("%1/Steam/steamapps/common/Starfield/Data").arg(programFiles));

        if (dataPaths.contains(game) && !dataPaths[game].isEmpty())
        {
            return QDir(dataPaths[game]);
        }

        return QDir();
    }

    static GameId detectGameFromVersion(float hedrVersion, quint32 incc)
    {
        // Order matters: check the most specific signatures first.
        // Skyrim AE/SE are uniquely identified by the combination of version
        // and incc, so they take precedence over the legacy range checks.
        if (hedrVersion >= 1.8f && incc == 17)
            return Game_SkyrimAnniversaryEdition;
        if (hedrVersion >= 1.6f && incc == 17)
            return Game_SkyrimSpecialEdition;
        // Legacy/fallback: non-Skyrim-family games use a synthetic high version
        // range (3.x=Morrowind, 4.x=Oblivion) when this function is called
        // outside its normal TES4 detection path. These checks must precede
        // the generic >= 1.5f Skyrim SE fallback so they remain reachable.
        if (hedrVersion >= 4.0f && hedrVersion < 5.0f)
            return Game_Oblivion;
        if (hedrVersion >= 3.0f && hedrVersion < 4.0f)
            return Game_Morrowind;
        if (hedrVersion >= 1.5f)
            return Game_SkyrimSpecialEdition;
        if (hedrVersion >= 0.94f && hedrVersion < 1.5f)
            return Game_Skyrim;
        return Game_None;
    }

    static QString gameName(GameId game)
    {
        switch (game)
        {
        case Game_Morrowind: return "Morrowind";
        case Game_Oblivion: return "Oblivion";
        case Game_Skyrim: return "Skyrim";
        case Game_SkyrimSpecialEdition: return "Skyrim Special Edition";
        case Game_SkyrimAnniversaryEdition: return "Skyrim Anniversary Edition";
        case Game_Fallout3: return "Fallout 3";
        case Game_FalloutNewVegas: return "Fallout: New Vegas";
        case Game_Fallout4: return "Fallout 4";
        case Game_Starfield: return "Starfield";
        default: return "Unknown";
        }
    }

    static QString dataDirKey(GameId game)
    {
        switch (game)
        {
        case Game_Morrowind: return "MorrowindDataDirectory";
        case Game_Oblivion: return "OblivionDataDirectory";
        case Game_Skyrim: return "SkyrimDataDirectory";
        case Game_SkyrimSpecialEdition:
        case Game_SkyrimAnniversaryEdition: return "SkyrimSEDataDirectory";
        case Game_Fallout3: return "Fallout3DataDirectory";
        case Game_FalloutNewVegas: return "FalloutNVDataDirectory";
        case Game_Fallout4: return "Fallout4DataDirectory";
        case Game_Starfield: return "StarfieldDataDirectory";
        default: return "DataDirectory";
        }
    }
    struct GameDetect {
        QString dataPath;
        GameId gameId;
    };

    // Returns the local AppData path for storage
    static QString localAppDataPath()
    {
#ifdef _WIN32
        return getEnvironmentVariable("LOCALAPPDATA");
#else
        return getEnvironmentVariable("HOME") + "/.local/share";
#endif
    }

    // Returns the centralized config file path (editor.ini)
    static QString configFilePath()
    {
        return QCoreApplication::applicationDirPath() + "/editor.ini";
    }

    // Returns the plugins.txt path for a given game
    static QStringList pluginsFilePaths(GameId game, const QString& dataPath)
    {
        QStringList paths;
        QString localApp = localAppDataPath();
        if (localApp.isEmpty())
            return paths;

        // Determine the game's AppData folder name
        QString gameFolder;
        QString msGameFolder;
        QString xboxPkgFolder;

        switch (game)
        {
        case Game_Oblivion:
            gameFolder = "Oblivion";
            msGameFolder = "Oblivion";
            break;
        case Game_Skyrim:
            gameFolder = "Skyrim";
            msGameFolder = "Skyrim";
            break;
        case Game_SkyrimSpecialEdition:
        case Game_SkyrimAnniversaryEdition:
            gameFolder = "Skyrim Special Edition";
            msGameFolder = "Skyrim Special Edition MS";
            xboxPkgFolder = "BethesdaSoftworks.SkyrimSE-PC_3275kfvn8vcwc";
            break;
        case Game_Fallout3:
            gameFolder = "Fallout3";
            msGameFolder = "Fallout3";
            break;
        case Game_FalloutNewVegas:
            gameFolder = "FalloutNV";
            msGameFolder = "FalloutNV";
            break;
        case Game_Fallout4:
            gameFolder = "Fallout4";
            msGameFolder = "Fallout4 MS";
            break;
        case Game_Starfield:
            gameFolder = "Starfield";
            msGameFolder = "Starfield";
            xboxPkgFolder = "BethesdaSoftworks.Starfield_3275kfvn8vcwc";
            break;
        default:
            return paths;
        }

        // Steam/GOG/Epic path
        QString steamPath = QString("%1/%2/plugins.txt").arg(localApp, gameFolder);
        paths.append(steamPath);

        // Xbox Game Pass path (via package folder)
        if (!xboxPkgFolder.isEmpty())
        {
            QString xboxPath = QString("%1/Packages/%2/LocalCache/Local/%3/plugins.txt")
                .arg(localApp, xboxPkgFolder, msGameFolder);
            paths.append(xboxPath);
        }

        // Fallback MS path
        if (msGameFolder != gameFolder)
        {
            QString msPath = QString("%1/%2/plugins.txt").arg(localApp, msGameFolder);
            paths.append(msPath);
        }

        return paths;
    }

    // Reads active plugins from plugins.txt
    static QStringList readActivePlugins(GameId game, const QString& dataPath)
    {
        QStringList activePlugins;
        QStringList possiblePaths = pluginsFilePaths(game, dataPath);

        // Games that use * prefix for active plugins
        bool useStarPrefix = (game == Game_SkyrimSpecialEdition || game == Game_SkyrimAnniversaryEdition || 
                              game == Game_Fallout4 || game == Game_Starfield);

        for (const auto& path : possiblePaths)
        {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;

            while (!file.atEnd())
            {
                QString line = QString::fromUtf8(file.readLine()).trimmed();
                if (line.isEmpty() || line.startsWith('#'))
                    continue;

                if (useStarPrefix)
                {
                    // Starfield/SSE/AE/FO4: only * prefixed entries are active
                    if (line.startsWith('*'))
                    {
                        activePlugins.append(line.mid(1));
                    }
                    // Non-* entries are inactive, ignore them
                }
                else
                {
                    // Oldrim/Oblivion/Fallout3/NV: all listed plugins are active
                    activePlugins.append(line);
                }
            }
            file.close();

            if (!activePlugins.isEmpty())
                break;
        }

        return activePlugins;
    }

    static QVector<GameDetect> detectGames()
    {
        QVector<GameDetect> found;
        QStringList searchPaths;

        // Steam library paths from all sources
        auto addSteamPath = [&](const QString& steamDir) {
            QString commonPath = QString("%1/steamapps/common").arg(steamDir);
            if (QDir(commonPath).exists())
                searchPaths.append(commonPath);
        };

#ifdef _WIN32
        // Check Steam registry for install path
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Valve\\Steam", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            char steamPath[MAX_PATH] = {};
            DWORD size = sizeof(steamPath);
            if (RegQueryValueExA(hKey, "InstallPath", nullptr, nullptr, (LPBYTE)steamPath, &size) == ERROR_SUCCESS)
            {
                QString steamDir = QString::fromUtf8(steamPath);
                addSteamPath(steamDir);

                // Parse libraryfolders.vdf for alternate libraries
                QString vdfPath = steamDir + "/steamapps/libraryfolders.vdf";
                QFile vdf(vdfPath);
                if (vdf.open(QIODevice::ReadOnly | QIODevice::Text))
                {
                    while (!vdf.atEnd())
                    {
                        QString line = QString::fromUtf8(vdf.readLine()).trimmed();
                        // Find quoted paths like: "1"    "D:\\SteamLibrary"
                        if (line.count('"') >= 4)
                        {
                            int firstQuote = line.indexOf('"', 1);
                            if (firstQuote > 0)
                            {
                                int secondQuote = line.indexOf('"', firstQuote + 1);
                                if (secondQuote > firstQuote)
                                {
                                    QString value = line.mid(firstQuote + 1, secondQuote - firstQuote - 1);
                                    if (value.contains(":") && value.contains("\\") && !value.contains("{"))
                                    {
                                        QString cleanPath = value.replace("\\", "/");
                                        addSteamPath(cleanPath);
                                    }
                                }
                            }
                        }
                    }
                    vdf.close();
                }
            }
            RegCloseKey(hKey);
        }

        // Also check common Steam paths
        QString programFiles = getEnvironmentVariable("PROGRAMFILES(X86)");
        if (programFiles.isEmpty())
            programFiles = "C:/Program Files (x86)";
        addSteamPath(programFiles + "/Steam");
        addSteamPath("C:/Program Files/Steam");

        // Check Xbox Game Pass (C:\XboxGames)
        QString xboxGamesPath = "C:/XboxGames";
        if (QDir(xboxGamesPath).exists())
        {
            QDirIterator xboxIt(xboxGamesPath, QDir::Dirs | QDir::NoDotAndDotDot);
            while (xboxIt.hasNext())
            {
                xboxIt.next();
                QString dirName = xboxIt.fileName();
                // Map Xbox folder names to game Data paths
                QString dataPath;
                GameId gameId = Game_None;

                // Check for Data subfolder (Xbox games have Content subfolder structure)
                QString contentPath = xboxIt.filePath() + "/Content";
                if (!QDir(contentPath).exists())
                    contentPath = xboxIt.filePath();

                if (dirName.contains("Skyrim Special Edition"))
                {
                    dataPath = contentPath + "/Data";
                    gameId = Game_SkyrimSpecialEdition;
                }
                else if (dirName.contains("Morrowind"))
                {
                    dataPath = contentPath + "/Data Files";
                    gameId = Game_Morrowind;
                }
                else if (dirName.contains("Oblivion"))
                {
                    dataPath = contentPath + "/Data";
                    gameId = Game_Oblivion;
                }
                else if (dirName.contains("Fallout 4"))
                {
                    dataPath = contentPath + "/Data";
                    gameId = Game_Fallout4;
                }
                else if (dirName.contains("Fallout 3"))
                {
                    dataPath = contentPath + "/Data";
                    gameId = Game_Fallout3;
                }
                else if (dirName.contains("Fallout New Vegas") || dirName.contains("FalloutNV"))
                {
                    dataPath = contentPath + "/Data";
                    gameId = Game_FalloutNewVegas;
                }
                else if (dirName == "Starfield")
                {
                    dataPath = contentPath + "/Data";
                    gameId = Game_Starfield;
                }

                if (gameId != Game_None && QDir(dataPath).exists())
                    found.append({dataPath, gameId});
            }
        }
#else
        addSteamPath(getEnvironmentVariable("HOME") + "/.local/share/Steam");
#endif

        // Define known game folder names for Steam/common paths
        struct FolderMap {
            QString name;
            GameId gameId;
        };
        QVector<FolderMap> knownFolders = {
            {"Skyrim", Game_Skyrim},
            {"Skyrim Special Edition", Game_SkyrimSpecialEdition},
            {"Skyrim Anniversary Edition", Game_SkyrimAnniversaryEdition},
            {"Oblivion", Game_Oblivion},
            {"Fallout 4", Game_Fallout4},
            {"Fallout 3 GOTY", Game_Fallout3},
            {"Fallout New Vegas", Game_FalloutNewVegas},
            {"Starfield", Game_Starfield},
        };

        // Search Steam library paths for known game folders
        for (const auto& basePath : searchPaths)
        {
            QDir baseDir(basePath);
            if (!baseDir.exists())
                continue;

            for (const auto& folder : knownFolders)
            {
                QString gameDir = baseDir.absolutePath() + "/" + folder.name;
                QString dataDir = gameDir + "/Data";
                if (QDir(dataDir).exists())
                {
                    found.append({dataDir, folder.gameId});
                }
            }
        }

        // Morrowind is special - check OpenMW path
        QString homeDir = getEnvironmentVariable("HOME");
        if (!homeDir.isEmpty())
        {
            QString mwPath = homeDir + "/.openmw/data";
            if (QDir(mwPath).exists())
            {
                bool alreadyFound = false;
                for (const auto& f : found)
                    if (f.gameId == Game_Morrowind) { alreadyFound = true; break; }
                if (!alreadyFound)
                    found.append({mwPath, Game_Morrowind});
            }
        }

        // Remove exact duplicate paths
        QVector<GameDetect> deduped;
        QSet<QString> seenPaths;
        for (const auto& f : found)
        {
            QString normalized = QDir::toNativeSeparators(f.dataPath).toLower();
            if (!seenPaths.contains(normalized))
            {
                seenPaths.insert(normalized);
                deduped.append(f);
            }
        }

        return deduped;
    }
};

#endif // FILEPATH_H
