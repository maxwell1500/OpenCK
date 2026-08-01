#include "meshlodconfig.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

#include "libs/files/log/logger.hpp"

bool MeshLodConfig::parse(const QString& json, MeshLodConfig& out)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError)
    {
        LOG_WARNING(QString("MeshLodConfig: JSON error: %1").arg(err.errorString()));
        return false;
    }

    QJsonArray levels;
    if (doc.isArray())
    {
        levels = doc.array();
    }
    else if (doc.isObject())
    {
        const QJsonObject root = doc.object();
        out.name = root.value(QStringLiteral("name")).toString();
        out.outputAssociation = root.value(QStringLiteral("outputAssociation")).toString(
            root.value(QStringLiteral("association")).toString());
        out.lodNamePattern = root.value(QStringLiteral("lodNamePattern")).toString(
            root.value(QStringLiteral("pattern")).toString());
        out.enabled = root.value(QStringLiteral("enabled")).toBool(true);
        if (root.contains(QStringLiteral("levels")))
            levels = root.value(QStringLiteral("levels")).toArray();
        else if (root.contains(QStringLiteral("lodLevels")))
            levels = root.value(QStringLiteral("lodLevels")).toArray();
        else
            return false;
    }
    else
    {
        return false;
    }

    for (const QJsonValue& lv : levels)
    {
        if (!lv.isObject())
            continue;
        const QJsonObject lobj = lv.toObject();
        LodLevel level;
        level.level = lobj.value(QStringLiteral("level")).toInt(
            lobj.value(QStringLiteral("index")).toInt(0));
        if (level.level <= 0)
            level.level = out.levels.size() + 1;
        level.screenSize = static_cast<float>(lobj.value(QStringLiteral("screenSize")).toDouble(
            lobj.value(QStringLiteral("coverage")).toDouble(0.05)));
        level.reductionPercent = static_cast<float>(lobj.value(QStringLiteral("reductionPercent")).toDouble(
            lobj.value(QStringLiteral("reduction")).toDouble(0.5)));
        level.maxTriangleCount = lobj.value(QStringLiteral("maxTriangles")).toInt(
            lobj.value(QStringLiteral("maxTriangleCount")).toInt(0));
        level.preserveUVs = lobj.value(QStringLiteral("preserveUVs")).toBool(true);
        level.preserveNormals = lobj.value(QStringLiteral("preserveNormals")).toBool(true);
        level.generateCollision = lobj.value(QStringLiteral("generateCollision")).toBool(false);
        out.levels.append(level);
    }

    return !out.levels.isEmpty();
}

bool MeshLodConfig::loadFile(const QString& path, MeshLodConfig& out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_WARNING(QString("MeshLodConfig: cannot open %1").arg(path));
        return false;
    }
    const QString json = QString::fromUtf8(file.readAll());
    return parse(json, out);
}

const MeshLodConfig::LodLevel* MeshLodConfig::levelForScreenSize(float screenSize) const
{
    // Levels are usually ordered high-to-low coverage; the first level whose
    // threshold the screen size falls below wins.
    const LodLevel* best = nullptr;
    for (const LodLevel& level : levels)
    {
        if (screenSize >= level.screenSize)
        {
            best = &level;
            break;
        }
        if (screenSize < level.screenSize)
            best = &level;
    }
    return best;
}

MeshLodConfig MeshLodConfig::builtin()
{
    MeshLodConfig config;
    config.name = QStringLiteral("Default");
    config.lodNamePattern = QStringLiteral("%1_LOD%2.nif");
    config.outputAssociation = QStringLiteral("lod_associations.json");

    LodLevel l1;
    l1.level = 1; l1.screenSize = 0.2f; l1.reductionPercent = 0.3f;
    config.levels.append(l1);

    LodLevel l2;
    l2.level = 2; l2.screenSize = 0.06f; l2.reductionPercent = 0.55f;
    config.levels.append(l2);

    LodLevel l3;
    l3.level = 3; l3.screenSize = 0.02f; l3.reductionPercent = 0.8f;
    l3.generateCollision = true;
    config.levels.append(l3);

    return config;
}
