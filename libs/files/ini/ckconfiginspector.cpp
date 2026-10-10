#include "ckconfiginspector.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QSet>

bool CkConfigInspector::loadFromDirectory(const QString& dirPath)
{
    m_colors.clear();
    m_loadedDir.clear();

    const QDir dir(dirPath);
    if (!dir.exists())
        return false;

    m_loadedDir = dir.absolutePath();

    const QString ckIniPath = dir.filePath(QStringLiteral("CreationKit.ini"));
    if (QFile::exists(ckIniPath))
        m_ckIni.load(ckIniPath);

    const QString prefsIniPath = dir.filePath(QStringLiteral("CreationKitPrefs.ini"));
    if (QFile::exists(prefsIniPath))
        m_prefsIni.load(prefsIniPath);

    const QString customIniPath = dir.filePath(QStringLiteral("CreationKitCustom.ini"));
    if (QFile::exists(customIniPath))
        m_customIni.load(customIniPath);

    const QString colorsPath = dir.filePath(QStringLiteral("EditorColors.xml"));
    if (QFile::exists(colorsPath))
        parseColorsXml(colorsPath);

    return !m_ckIni.isEmpty() || !m_prefsIni.isEmpty() || !m_colors.isEmpty();
}

QString CkConfigInspector::detectCreationKitDirectory()
{
    const QString dataEnv = qEnvironmentVariable("OPENCK_DATA_DIR");
    if (!dataEnv.isEmpty())
    {
        const QDir dataDir(dataEnv);
        const QString parentDir = dataDir.filePath(QStringLiteral(".."));
        if (QFile::exists(QDir(parentDir).filePath(QStringLiteral("CreationKit.ini"))))
            return QDir(parentDir).absolutePath();
    }

    const QStringList candidates = {
        QStringLiteral("C:/XboxGames/Starfield/Content"),
        QStringLiteral("C:/Program Files (x86)/Steam/steamapps/common/Starfield"),
        QStringLiteral("C:/Program Files (x86)/Steam/steamapps/common/Skyrim Special Edition"),
        QStringLiteral("C:/Program Files (x86)/Steam/steamapps/common/Fallout 4")
    };

    for (const QString& candidate : candidates)
    {
        if (QFile::exists(candidate + QStringLiteral("/CreationKit.ini")))
            return candidate;
    }

    return QString();
}

QStringList CkConfigInspector::resourceArchiveList() const
{
    QStringList list = m_customIni.valueList(QStringLiteral("Archive"), QStringLiteral("SResourceArchiveList"));
    if (list.isEmpty())
        list = m_ckIni.valueList(QStringLiteral("Archive"), QStringLiteral("SResourceArchiveList"));
    if (list.isEmpty())
        list = m_prefsIni.valueList(QStringLiteral("Archive"), QStringLiteral("SResourceArchiveList"));
    return list;
}

QStringList CkConfigInspector::resourceIndexFileList() const
{
    QStringList list = m_customIni.valueList(QStringLiteral("Archive"), QStringLiteral("sResourceIndexFileList"));
    if (list.isEmpty())
        list = m_ckIni.valueList(QStringLiteral("Archive"), QStringLiteral("sResourceIndexFileList"));
    if (list.isEmpty())
        list = m_prefsIni.valueList(QStringLiteral("Archive"), QStringLiteral("sResourceIndexFileList"));
    return list;
}

QStringList CkConfigInspector::allArchives() const
{
    QSet<QString> set;
    for (const QString& s : resourceArchiveList())
        set.insert(s.trimmed());
    for (const QString& s : resourceIndexFileList())
        set.insert(s.trimmed());

    QStringList result = set.values();
    result.sort(Qt::CaseInsensitive);
    return result;
}

float CkConfigInspector::fov(float defaultValue) const
{
    const QString val = m_prefsIni.value(QStringLiteral("Display"), QStringLiteral("fDefaultFOV"),
        m_ckIni.value(QStringLiteral("Display"), QStringLiteral("fDefaultFOV")));
    if (val.isEmpty())
        return defaultValue;
    bool ok = false;
    const float f = val.toFloat(&ok);
    return ok ? f : defaultValue;
}

float CkConfigInspector::cameraSpeed(float defaultValue) const
{
    const QString val = m_prefsIni.value(QStringLiteral("Display"), QStringLiteral("fCameraSpeed"),
        m_ckIni.value(QStringLiteral("Display"), QStringLiteral("fCameraSpeed")));
    if (val.isEmpty())
        return defaultValue;
    bool ok = false;
    const float f = val.toFloat(&ok);
    return ok ? f : defaultValue;
}

QString CkConfigInspector::papyrusCompiler(const QString& defaultValue) const
{
    const QString val = m_ckIni.value(QStringLiteral("Papyrus"), QStringLiteral("sScriptCompiler"));
    return val.isEmpty() ? defaultValue : val;
}

QStringList CkConfigInspector::papyrusSourceFolders() const
{
    return m_ckIni.valueList(QStringLiteral("Papyrus"), QStringLiteral("sScriptSourceFolder"));
}

QStringList CkConfigInspector::papyrusAdditionalImports() const
{
    return m_ckIni.valueList(QStringLiteral("Papyrus"), QStringLiteral("sAdditionalImports"));
}

QColor CkConfigInspector::color(const QString& element, const QString& slot, const QColor& defaultColor) const
{
    if (!m_colors.contains(element))
        return defaultColor;

    const CkColorEntry& entry = m_colors.value(element);
    if (slot.compare(QStringLiteral("Primary"), Qt::CaseInsensitive) == 0)
        return entry.primary.isValid() ? entry.primary : defaultColor;
    if (slot.compare(QStringLiteral("Secondary"), Qt::CaseInsensitive) == 0)
        return entry.secondary.isValid() ? entry.secondary : defaultColor;
    if (slot.compare(QStringLiteral("Third"), Qt::CaseInsensitive) == 0)
        return entry.third.isValid() ? entry.third : defaultColor;
    if (slot.compare(QStringLiteral("Fourth"), Qt::CaseInsensitive) == 0)
        return entry.fourth.isValid() ? entry.fourth : defaultColor;
    if (slot.compare(QStringLiteral("Fifth"), Qt::CaseInsensitive) == 0)
        return entry.fifth.isValid() ? entry.fifth : defaultColor;

    return defaultColor;
}

bool CkConfigInspector::parseColorsXml(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QXmlStreamReader xml(&file);
    while (!xml.atEnd() && !xml.hasError())
    {
        xml.readNext();
        if (xml.isStartElement() && xml.name() != QLatin1String("root"))
        {
            const QString element = xml.name().toString();
            CkColorEntry entry;

            while (!xml.atEnd())
            {
                xml.readNext();
                if (xml.isStartElement())
                {
                    const QString slot = xml.name().toString();
                    int r = 0, g = 0, b = 0, a = 255;
                    while (!xml.atEnd())
                    {
                        xml.readNext();
                        if (xml.isStartElement())
                        {
                            const QString ch = xml.name().toString();
                            const QString txt = xml.readElementText();
                            if (ch.compare(QLatin1String("R"), Qt::CaseInsensitive) == 0) r = txt.toInt();
                            else if (ch.compare(QLatin1String("G"), Qt::CaseInsensitive) == 0) g = txt.toInt();
                            else if (ch.compare(QLatin1String("B"), Qt::CaseInsensitive) == 0) b = txt.toInt();
                            else if (ch.compare(QLatin1String("A"), Qt::CaseInsensitive) == 0) a = txt.toInt();
                        }
                        else if (xml.isEndElement() && xml.name().toString() == slot)
                        {
                            break;
                        }
                    }

                    const QColor c = QColor::fromRgb(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255), qBound(0, a, 255));
                    if (slot.compare(QLatin1String("Primary"), Qt::CaseInsensitive) == 0) entry.primary = c;
                    else if (slot.compare(QLatin1String("Secondary"), Qt::CaseInsensitive) == 0) entry.secondary = c;
                    else if (slot.compare(QLatin1String("Third"), Qt::CaseInsensitive) == 0) entry.third = c;
                    else if (slot.compare(QLatin1String("Fourth"), Qt::CaseInsensitive) == 0) entry.fourth = c;
                    else if (slot.compare(QLatin1String("Fifth"), Qt::CaseInsensitive) == 0) entry.fifth = c;
                }
                else if (xml.isEndElement() && xml.name().toString() == element)
                {
                    break;
                }
            }

            m_colors.insert(element, entry);
        }
    }

    return !m_colors.isEmpty();
}
