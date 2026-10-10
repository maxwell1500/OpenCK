#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QColor>
#include "inifile.hpp"

struct CkColorEntry
{
    QColor primary;
    QColor secondary;
    QColor third;
    QColor fourth;
    QColor fifth;
};

// CkConfigInspector provides read-only inspection and import for Creation Kit
// configuration files: CreationKit.ini, CreationKitPrefs.ini, CreationKitCustom.ini,
// and EditorColors.xml, as well as archive lists and toolchain paths.
class CkConfigInspector
{
public:
    CkConfigInspector() = default;

    // Loads CreationKit.ini, CreationKitPrefs.ini, CreationKitCustom.ini,
    // and EditorColors.xml from the specified directory or install path.
    bool loadFromDirectory(const QString& dirPath);

    // Attempts to locate installed CreationKit directory from OPENCK_DATA_DIR,
    // Starfield content directory, or common Steam/Xbox locations.
    static QString detectCreationKitDirectory();

    // INI Files
    const IniFile& creationKitIni() const { return m_ckIni; }
    const IniFile& prefsIni() const { return m_prefsIni; }
    const IniFile& customIni() const { return m_customIni; }

    // Archive lists (from [Archive])
    QStringList resourceArchiveList() const;
    QStringList resourceIndexFileList() const;
    QStringList allArchives() const;

    // Display settings (from [Display])
    float fov(float defaultValue = 75.0f) const;
    float cameraSpeed(float defaultValue = 1.0f) const;

    // Papyrus settings (from [Papyrus])
    QString papyrusCompiler(const QString& defaultValue = QString()) const;
    QStringList papyrusSourceFolders() const;
    QStringList papyrusAdditionalImports() const;

    // Colors (from EditorColors.xml)
    bool hasColors() const { return !m_colors.isEmpty(); }
    QMap<QString, CkColorEntry> colors() const { return m_colors; }
    QColor color(const QString& element, const QString& slot = QStringLiteral("Primary"),
                 const QColor& defaultColor = QColor()) const;

    QString loadedDirectory() const { return m_loadedDir; }

private:
    bool parseColorsXml(const QString& filePath);

    IniFile m_ckIni;
    IniFile m_prefsIni;
    IniFile m_customIni;
    QMap<QString, CkColorEntry> m_colors;
    QString m_loadedDir;
};
