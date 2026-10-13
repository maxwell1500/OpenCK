#include "matfile.hpp"

#include "../data/zipreader.hpp"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QVector>
#include <QRegularExpression>
#include <QStringList>

namespace {

struct MatObject
{
    QString name;
    QString parent;
    QMap<int, QString> files; // slot index -> texture file name
};

// An object's role is carried by its name ("<Family>_TextureSet1"), which is
// the only reliable marker of either schema: a loose mod .mat names the parent
// file ("...\\texturesets.mat") while the Creation Kit's dumps it as a
// resource id ("res:DC239E25:...").
const QRegularExpression textureSetNameRe(QStringLiteral("_TextureSet\\d+$", QRegularExpression::CaseInsensitiveOption));
const QRegularExpression roleSuffixRe(QStringLiteral("_(Material|TextureSet|Layer|Blender|UVStream)\\d+$", QRegularExpression::CaseInsensitiveOption));

bool isTextureSet(const MatObject& o)
{
    if (o.name.contains(textureSetNameRe))
        return true;
    // Fall back to the parent-file spelling used by loose files.
    return o.parent.endsWith(QLatin1String("texturesets.mat"), Qt::CaseInsensitive);
}

// The object named after the family alone is the root the sibling texture sets
// are prefixed with.
bool isLayeredRoot(const MatObject& o)
{
    return !o.name.isEmpty() && !o.name.contains(roleSuffixRe);
}

QString nameOf(const QJsonObject& component)
{
    return component.value(QLatin1String("Data")).toObject()
        .value(QLatin1String("Name")).toString();
}

// Reads a texture file name out of a component, or an empty string. Material
// parameters that happen to be called FileName (shader models such as
// "4LayerStandard") are dropped because they are not textures.
QString textureFileOf(const QJsonObject& component)
{
    const QString fn = component.value(QLatin1String("Data")).toObject()
                          .value(QLatin1String("FileName")).toString();
    if (fn.endsWith(QLatin1String(".dds"), Qt::CaseInsensitive) ||
        fn.endsWith(QLatin1String(".tga"), Qt::CaseInsensitive))
        return fn;
    return QString();
}

QVector<MatObject> readObjects(const QByteArray& json)
{
    QVector<MatObject> out;
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return out;

    const QJsonArray objs = doc.object().value(QLatin1String("Objects")).toArray();
    out.reserve(objs.size());
    for (int i = 0; i < objs.size(); ++i) {
        const QJsonObject oo = objs.at(i).toObject();
        MatObject o;
        o.parent = oo.value(QLatin1String("Parent")).toString();
        const QJsonArray comps = oo.value(QLatin1String("Components")).toArray();
        for (int c = 0; c < comps.size(); ++c) {
            const QJsonObject co = comps.at(c).toObject();
            if (co.value(QLatin1String("Type")).toString() ==
                QLatin1String("BSComponentDB::CTName"))
                o.name = nameOf(co);
            const QString fn = textureFileOf(co);
            if (!fn.isEmpty())
                o.files.insert(co.value(QLatin1String("Index")).toInt(), fn);
        }
        out.append(o);
    }
    return out;
}

// Merges every texture set belonging to `family` into `out`.
void collectFamily(const QVector<MatObject>& objects, const QString& family,
                   QMap<int, QString>& out)
{
    const QString prefix = family + QLatin1Char('_');
    for (int i = 0; i < objects.size(); ++i) {
        const MatObject& ts = objects.at(i);
        if (!isTextureSet(ts) || !ts.name.startsWith(prefix))
            continue;
        for (auto it = ts.files.constBegin(); it != ts.files.constEnd(); ++it)
            out.insert(it.key(), it.value());
    }
}

} // namespace

// The Creation Kit redistributes the base game's authored .mat files as JSON in
// Tools\ContentResources.zip (~48k files). Indexing it costs one pass over the
// central directory, so it is opened on first use rather than at startup.
namespace {
QString g_contentResourcesPath;
ZipReader* g_contentResources = nullptr;
bool g_contentResourcesTried = false;

ZipReader* contentResources()
{
    if (!g_contentResourcesTried) {
        g_contentResourcesTried = true;
        if (!g_contentResourcesPath.isEmpty())
            g_contentResources = new ZipReader(g_contentResourcesPath);
    }
    return g_contentResources;
}
} // namespace

QMap<int, QString> MatFile::textureSlots(const QString& absPath)
{
    QMap<int, QString> out;
    if (absPath.isEmpty())
        return out;

    QFile f(absPath);
    if (!f.open(QIODevice::ReadOnly))
        return out;

    const QVector<MatObject> objects = readObjects(f.readAll());
    if (objects.isEmpty())
        return out;

    // Take the root layered material's family first so that LOD-only texture
    // sets cannot shadow the full-resolution one.
    for (int i = 0; i < objects.size(); ++i) {
        const MatObject& root = objects.at(i);
        if (!isLayeredRoot(root) || root.name.isEmpty())
            continue;
        collectFamily(objects, root.name, out);
        if (!out.isEmpty())
            return out;
        // A root may be a variation on an existing material file, so its
        // texture sets are named after that file rather than after it
        // ("DecalMetalBevels01n" -> "DecalMetalBevels01_TextureSet1").
        const QString stem = QFileInfo(root.parent).completeBaseName();
        if (!stem.isEmpty() && stem.compare(root.name, Qt::CaseInsensitive) != 0) {
            collectFamily(objects, stem, out);
            if (!out.isEmpty())
                return out;
        }
    }

    // Single-material files declare no layered root, so use whatever is there.
    for (int i = 0; i < objects.size(); ++i) {
        if (!isTextureSet(objects.at(i)))
            continue;
        for (auto it = objects.at(i).files.constBegin();
             it != objects.at(i).files.constEnd(); ++it)
            out.insert(it.key(), it.value());
    }
    return out;
}

QString MatFile::findLoose(const QString& materialPath, const QString& dataDir)
{
    if (materialPath.isEmpty() || dataDir.isEmpty())
        return QString();

    // Mods are inconsistent about doubled separators ("Materials\\morrowind\..")
    // so collapse runs of either separator before rebuilding the relative path.
    static const QRegularExpression sepRun(QStringLiteral("[\\\\/]+"));
    const QString rel = QString(materialPath).replace(sepRun, QStringLiteral("\\"));

    const QString direct = dataDir + QLatin1Char('/') + rel;
    if (QFileInfo::exists(direct))
        return direct;

    // A mod may also ship an entirely different directory layout, so fall back
    // to any file with the same base name under materials\.
    const QString base = QFileInfo(rel).fileName();
    QDirIterator it(dataDir + QLatin1String("/materials"), QStringList() << base,
                    QDir::Files, QDirIterator::Subdirectories);
    if (it.hasNext())
        return it.next();

    // Base-game materials ship only inside the Creation Kit's
    // ContentResources.zip, so pull out the single entry we need and cache it
    // next to the extracted meshes and textures.
    if (ZipReader* zr = contentResources()) {
        const QString cache = QDir::cleanPath(dataDir + QLatin1String("/.openck_cache/materials/") + rel);
        if (QFileInfo::exists(cache))
            return cache;

        // Entries in the archive use forward slashes.
        const QByteArray body = zr->readEntry(QString(rel).replace(QLatin1Char('\\'), QLatin1Char('/')));
        if (body.isEmpty())
            return QString();

        QDir().mkpath(QFileInfo(cache).absolutePath());
        QFile out(cache);
        if (!out.open(QIODevice::WriteOnly) || out.write(body) != body.size())
            return QString();
        return cache;
    }
    return QString();
}

void MatFile::setContentResources(const QString& zipPath)
{
    if (g_contentResourcesPath != zipPath) {
        g_contentResourcesPath = zipPath;
        delete g_contentResources;
        g_contentResources = nullptr;
        g_contentResourcesTried = false;
    }
}

QString MatFile::colorTexture(const QString& materialPath, const QString& dataDir)
{
    const QString mat = findLoose(materialPath, dataDir);
    if (mat.isEmpty())
        return QString();

    // Slot 0 is the albedo map. See MatChannel for the full measured map and
    // why no other slot may stand in for it.
    QString tex = textureSlots(mat).value(MatChannel_Albedo);
    if (tex.isEmpty())
        return QString();

    // .mat texture paths are written relative to the install root
    // ("Data\Textures\...") while dataDir IS that Data folder, so the leading
    // component has to go for the asset resolver to find the file.
    static const QRegularExpression sepRun(QStringLiteral("[\\\\/]+"));
    const QStringList parts = QString(tex).replace(sepRun, QStringLiteral("\\"))
                                     .split(QLatin1Char('\\'), Qt::SkipEmptyParts);
    if (parts.size() > 1 && parts.first().compare(QLatin1String("data"),
                                                 Qt::CaseInsensitive) == 0)
        return parts.mid(1).join(QLatin1Char('\\'));
    return tex;
}
