#include "matfile.hpp"

#include "../data/zipreader.hpp"
#include "matdatabase.hpp"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QVector>
#include <QRegularExpression>
#include <algorithm>
#include <QStringList>
#include <QColor>
#include <QHash>
#include <QSet>

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
const QRegularExpression textureSetNameRe(QStringLiteral("_TextureSet\\d+$"), QRegularExpression::CaseInsensitiveOption);
const QRegularExpression roleSuffixRe(QStringLiteral("_(Material|TextureSet|Layer|Blender|UVStream)\\d+$"), QRegularExpression::CaseInsensitiveOption);

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

QString g_materialDatabasePath;
MatDatabase* g_materialDatabase = nullptr;
bool g_materialDatabaseTried = false;

ZipReader* contentResources()
{
    if (!g_contentResourcesTried) {
        g_contentResourcesTried = true;
        if (!g_contentResourcesPath.isEmpty())
            g_contentResources = new ZipReader(g_contentResourcesPath);
    }
    return g_contentResources;
}

// What a material resolves to once its layer graph has been walked.
struct MatAppearance
{
    QString albedo;     // install-relative texture path, empty if none
    QString opacity;    // alpha mask (slot 2), empty if none
    QColor tint;        // multiplier for the albedo; invalid when untinted
};
// shortcut: keyed by resolved .mat path, never invalidated by edits to a loose
// file while the app runs; reopen the cell to pick up a changed material.
QHash<QString, MatAppearance> g_appearanceCache;
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

namespace {
MatDatabase* materialDatabase()
{
    if (!g_materialDatabaseTried) {
        g_materialDatabaseTried = true;
        if (!g_materialDatabasePath.isEmpty())
            g_materialDatabase = new MatDatabase(g_materialDatabasePath);
    }
    return g_materialDatabase;
}

// The texture set a material owns is named after the material file, the same
// convention the loose .mat files follow.
QString materialDocument(const QString& rel)
{
    MatDatabase* db = materialDatabase();
    if (!db)
        return QString();

    const QString base = QFileInfo(rel).completeBaseName();
    return db->matDocument(base + QStringLiteral("_TextureSet1"));
}
} // namespace

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
    // next to the extracted meshes and textures. The zip holds the full layered
    // graph, so it is tried before the compiled database below, which can only
    // supply a bare texture set.
    const QString cache = QDir::cleanPath(dataDir + QLatin1String("/.openck_cache/materials/") + rel);
    if (QFileInfo::exists(cache))
        return cache;

    if (ZipReader* zr = contentResources()) {
        // Entries in the archive use forward slashes.
        const QByteArray body = zr->readEntry(QString(rel).replace(QLatin1Char('\\'), QLatin1Char('/')));
        if (!body.isEmpty()) {
            QDir().mkpath(QFileInfo(cache).absolutePath());
            QFile out(cache);
            if (out.open(QIODevice::WriteOnly) && out.write(body) == body.size())
                return cache;
            return QString();
        }
    }

    // Materials in neither place live only in the compiled database. Read the
    // texture set out of it and write it in the same JSON a loose file uses, so
    // textureSlots() reads it unchanged.
    const QString doc = materialDocument(rel);
    if (!doc.isEmpty()) {
        QDir().mkpath(QFileInfo(cache).absolutePath());
        QFile out(cache);
        const QByteArray bytes = doc.toUtf8();
        if (out.open(QIODevice::WriteOnly) && out.write(bytes) == bytes.size())
            return cache;
    }
    return QString();
}

void MatFile::setContentResources(const QString& zipPath)
{
    g_appearanceCache.clear();
    if (g_contentResourcesPath != zipPath) {
        g_contentResourcesPath = zipPath;
        delete g_contentResources;
        g_contentResources = nullptr;
        g_contentResourcesTried = false;
    }
}

void MatFile::setMaterialDatabase(const QString& cdbPath)
{
    g_appearanceCache.clear();
    if (g_materialDatabasePath != cdbPath) {
        g_materialDatabasePath = cdbPath;
        delete g_materialDatabase;
        g_materialDatabase = nullptr;
        g_materialDatabaseTried = false;
    }
}

namespace {

// "Data\\Textures\\x.dds" -> "Textures\\x.dds". .mat paths are written
// relative to the install root while dataDir IS the Data folder, so the leading
// component has to go for the asset resolver and findLoose() to find the file.
QString dataRelative(const QString& p)
{
    static const QRegularExpression sepRun(QStringLiteral("[\\\\/]+"));
    const QStringList parts = QString(p).replace(sepRun, QStringLiteral("\\"))
                                  .split(QLatin1Char('\\'), Qt::SkipEmptyParts);
    if (parts.size() > 1 && parts.first().compare(QLatin1String("data"), Qt::CaseInsensitive) == 0)
        return parts.mid(1).join(QLatin1Char('\\'));
    return p;
}

// A layered material is a graph of objects spread over several files: the
// root names a file as its Parent and lists LayerIDs; each layer names a
// material, which names a texture set. Objects reference each other by "res:"
// id, ids are unique across the whole set, and an object that does not carry a
// component itself inherits it from the object its own Parent id names (often
// in another file). A variation such as "PaintedMetalPanel01_Grey01" therefore
// has texture sets with no files of their own.
using ObjectIndex = QHash<QString, QJsonObject>; // lowercased "res:" id -> object

constexpr int kMaxFiles = 64;     // an Import chain is a handful of files
constexpr int kMaxInherit = 32;   // Parent chains are a handful of objects

// Indexes `path` and everything it Imports. Objects of `path` itself are
// returned through rootObjects.
void indexClosure(const QString& path, const QString& dataDir, QSet<QString>& seen,
                  ObjectIndex& ids, QJsonArray* rootObjects)
{
    const QString key = QDir::cleanPath(path).toLower();
    if (seen.contains(key) || seen.size() >= kMaxFiles)
        return;
    seen.insert(key);

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QJsonObject doc = QJsonDocument::fromJson(f.readAll()).object();

    const QJsonArray objs = doc.value(QLatin1String("Objects")).toArray();
    for (const QJsonValue& v : objs) {
        const QJsonObject o = v.toObject();
        const QString id = o.value(QLatin1String("ID")).toString();
        if (!id.isEmpty())
            ids.insert(id.toLower(), o);
    }
    if (rootObjects)
        *rootObjects = objs;

    const QJsonArray imports = doc.value(QLatin1String("Import")).toArray();
    for (const QJsonValue& imp : imports) {
        const QString p = MatFile::findLoose(dataRelative(imp.toString()), dataDir);
        if (!p.isEmpty())
            indexClosure(p, dataDir, seen, ids, nullptr);
    }
}

// The material's own object: it names a file as Parent and has no id, because
// nothing references it.
QJsonObject findRoot(const QJsonArray& objs)
{
    for (const QJsonValue& v : objs) {
        const QJsonObject o = v.toObject();
        if (o.value(QLatin1String("Parent")).toString().endsWith(QLatin1String(".mat"), Qt::CaseInsensitive)
            && !o.contains(QLatin1String("ID")))
            return o;
    }
    return QJsonObject();
}

QJsonObject parentOf(const QJsonObject& o, const ObjectIndex& ids)
{
    const QString p = o.value(QLatin1String("Parent")).toString().toLower();
    return p.startsWith(QLatin1String("res:")) ? ids.value(p) : QJsonObject();
}

QJsonObject referenced(const QJsonObject& component, const ObjectIndex& ids)
{
    return ids.value(component.value(QLatin1String("Data")).toObject()
                         .value(QLatin1String("ID")).toString().toLower());
}

// The components of `type` on the nearest object up the Parent chain that has
// any, in Index order.
QVector<QJsonObject> inheritedComponents(QJsonObject o, const char* type, const ObjectIndex& ids)
{
    for (int depth = 0; !o.isEmpty() && depth < kMaxInherit; ++depth) {
        QVector<QJsonObject> out;
        const QJsonArray comps = o.value(QLatin1String("Components")).toArray();
        for (const QJsonValue& c : comps) {
            const QJsonObject co = c.toObject();
            if (co.value(QLatin1String("Type")).toString() == QLatin1String(type))
                out.append(co);
        }
        if (!out.isEmpty()) {
            std::sort(out.begin(), out.end(), [](const QJsonObject& a, const QJsonObject& b) {
                return a.value(QLatin1String("Index")).toInt() < b.value(QLatin1String("Index")).toInt();
            });
            return out;
        }
        o = parentOf(o, ids);
    }
    return {};
}

QMap<int, QString> inheritedFiles(QJsonObject o, const ObjectIndex& ids)
{
    for (int depth = 0; !o.isEmpty() && depth < kMaxInherit; ++depth) {
        QMap<int, QString> files;
        const QJsonArray comps = o.value(QLatin1String("Components")).toArray();
        for (const QJsonValue& c : comps) {
            const QJsonObject co = c.toObject();
            const QString fn = textureFileOf(co);
            if (!fn.isEmpty())
                files.insert(co.value(QLatin1String("Index")).toInt(), fn);
        }
        if (!files.isEmpty())
            return files;
        o = parentOf(o, ids);
    }
    return {};
}

double number(const QJsonValue& v)
{
    return v.isString() ? v.toString().toDouble() : v.toDouble();
}

struct LayerInfo
{
    QMap<int, QString> files;
    bool hasColor = false;
    double r = 1, g = 1, b = 1, w = 1;  // w: how strongly the tint applies
};

QVector<LayerInfo> layersOf(const QJsonObject& root, const ObjectIndex& ids)
{
    QVector<LayerInfo> out;
    for (const QJsonObject& lc : inheritedComponents(root, "BSMaterial::LayerID", ids)) {
        LayerInfo li;
        const QJsonObject layer = referenced(lc, ids);
        const QVector<QJsonObject> mc = inheritedComponents(layer, "BSMaterial::MaterialID", ids);
        const QJsonObject mat = mc.isEmpty() ? QJsonObject() : referenced(mc.first(), ids);
        const QVector<QJsonObject> tc = inheritedComponents(mat, "BSMaterial::TextureSetID", ids);
        if (!tc.isEmpty())
            li.files = inheritedFiles(referenced(tc.first(), ids), ids);

        const QVector<QJsonObject> cc = inheritedComponents(mat, "BSMaterial::Color", ids);
        if (!cc.isEmpty()) {
            const QJsonObject c = cc.first().value(QLatin1String("Data")).toObject()
                                      .value(QLatin1String("Value")).toObject()
                                      .value(QLatin1String("Data")).toObject();
            li.hasColor = c.contains(QLatin1String("x"));
            li.r = number(c.value(QLatin1String("x")));
            li.g = number(c.value(QLatin1String("y")));
            li.b = number(c.value(QLatin1String("z")));
            if (c.contains(QLatin1String("w")))
                li.w = number(c.value(QLatin1String("w")));
        }
        out.append(li);
    }
    return out;
}

// A layer's colour map. Slot 0, or slot 7 when it holds a colour map: unlit and
// emissive effect materials (GlowDecal_*) file their colour there. Measured over
// the 516 base layers that have slot 7 and no slot 0, 225 are "_color" files
// and 268 are "_emissive" glow masks, so the file's own suffix is the test; an
// emissive mask must not be drawn as albedo.
QString albedoOf(const QMap<int, QString>& files)
{
    if (files.contains(MatChannel_Albedo))
        return files.value(MatChannel_Albedo);
    const QString emissive = files.value(MatChannel_Emissive);
    if (QFileInfo(emissive).completeBaseName().endsWith(QLatin1String("_color"), Qt::CaseInsensitive))
        return emissive;
    return QString();
}

// The first layer is the base. Its albedo is the colour; the later layers are
// detail (grunge, scratches, a metal under paint) mixed in by masks this
// viewer does not evaluate, so their albedo must not stand in for the base.
// A painted base has no albedo at all, only a Color: that colour is the surface.
// shortcut: a base with no albedo and no Color at all (eyes: iris on layer 2)
// takes the first later layer's albedo; the blend masks are not read.
MatAppearance appearanceOf(const QVector<LayerInfo>& layers)
{
    MatAppearance out;
    if (layers.isEmpty())
        return out;

    const LayerInfo& base = layers.first();
    out.albedo = albedoOf(base.files);
    out.opacity = base.files.value(MatChannel_Opacity);
    if (out.albedo.isEmpty() && !base.hasColor) {
        for (int i = 1; i < layers.size() && out.albedo.isEmpty(); ++i)
            out.albedo = albedoOf(layers.at(i).files);
    }

    // shortcut: w is read as the tint strength, which is inferred, not read from
    // the CK. Measured: w is independent of the layer's override colour type
    // (Multiply occurs with w=0 and w=1 about equally). Over an albedo, w=0 means
    // "no tint": that never blackens a surface (the glow decals are black with
    // w=0). With no albedo the colour is all there is, so w only scales it when
    // positive, and a black w=0 colour (50 base materials) is taken as unset.
    // Upgrade when the component's field is documented.
    const bool unsetBlack = base.w == 0 && base.r == 0 && base.g == 0 && base.b == 0;
    const bool overAlbedo = !out.albedo.isEmpty();
    const bool apply = base.hasColor && (overAlbedo ? base.w > 0 : !unsetBlack);
    if (apply) {
        const double s = base.w > 0 ? base.w : 1.0;
        auto mix = [&](double c) { return qBound(0.0, 1.0 + (c - 1.0) * s, 1.0); };
        out.tint = QColor::fromRgbF(mix(base.r), mix(base.g), mix(base.b));
    }
    return out;
}

MatAppearance resolveAppearance(const QString& materialPath, const QString& dataDir)
{
    const QString mat = MatFile::findLoose(materialPath, dataDir);
    if (mat.isEmpty())
        return {};

    const auto cached = g_appearanceCache.constFind(mat);
    if (cached != g_appearanceCache.constEnd())
        return cached.value();

    QSet<QString> seen;
    ObjectIndex ids;
    QJsonArray rootObjects;
    indexClosure(mat, dataDir, seen, ids, &rootObjects);

    MatAppearance out;
    const QJsonObject root = findRoot(rootObjects);
    const QVector<LayerInfo> layers = root.isEmpty() ? QVector<LayerInfo>() : layersOf(root, ids);
    if (!layers.isEmpty()) {
        out = appearanceOf(layers);
    } else {
        // No layer graph: a loose single-material file, or a texture set the
        // compiled database synthesised. Read the texture sets directly.
        // Slot 0 is the albedo map. See MatChannel for the full measured map
        // and why no other slot may stand in for it.
        out.albedo = MatFile::textureSlots(mat).value(MatChannel_Albedo);
    }
    if (!out.albedo.isEmpty())
        out.albedo = dataRelative(out.albedo);
    if (!out.opacity.isEmpty())
        out.opacity = dataRelative(out.opacity);

    g_appearanceCache.insert(mat, out);
    return out;
}

} // namespace

QString MatFile::colorTexture(const QString& materialPath, const QString& dataDir)
{
    return resolveAppearance(materialPath, dataDir).albedo;
}

QColor MatFile::baseTint(const QString& materialPath, const QString& dataDir)
{
    return resolveAppearance(materialPath, dataDir).tint;
}

QString MatFile::opacityTexture(const QString& materialPath, const QString& dataDir)
{
    return resolveAppearance(materialPath, dataDir).opacity;
}
