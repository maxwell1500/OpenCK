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
};

#endif // MODMANAGERDETECTION_HPP
