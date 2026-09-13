#ifndef MODMANAGERDETECTION_HPP
#define MODMANAGERDETECTION_HPP

#include <QString>
#include <QStringList>

class ModManagerDetection
{
public:
    enum class ModManager { None, MO2, Vortex, Unknown };

    struct ModManagerInfo
    {
        ModManager type = ModManager::None;
        QString installPath;
        QString version;
        QString gamePath;
        QString modsDirectory;
        QString selectedProfile;
        QStringList profiles;
        bool isRunning = false;
    };

    static ModManagerInfo detect();
    static ModManagerInfo detectMO2();
    static ModManagerInfo detectVortex();
    static bool isMO2Running();
    static bool isVortexRunning();
    static QString getMO2Profile();
    static QString getVortexProfile();
    static QStringList getInstalledMods(ModManager manager);

    // Pure, file-based parsers (testable without a live install).
    static bool parseMo2Ini(const QString& iniPath, ModManagerInfo& out);
    static QString parseVortexField(const QString& jsonContent, const QString& key);
};

#endif // MODMANAGERDETECTION_HPP
