#include "materialpropertygraph.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

#include "libs/files/log/logger.hpp"

QVector<MaterialTextureSlot> MaterialPropertyGraph::standardSlots()
{
    QVector<MaterialTextureSlot> out;
    const auto add = [&](const char* name, const char* key, bool mandatory, bool optional) {
        MaterialTextureSlot s;
        s.name = QString::fromLatin1(name);
        s.textureKey = QString::fromLatin1(key);
        s.mandatory = mandatory;
        s.optional = optional;
        out.append(s);
    };

    add("Albedo", "Diffuse", true, false);
    add("Normal", "Normal", false, true);
    add("Roughness", "Roughness", false, true);
    add("Metalness", "Metalness", false, true);
    add("AO", "AmbientOcclusion", false, true);
    add("Curvature", "Curvature", false, true);
    add("Height", "Height", false, true);
    add("Emissive", "Emissive", false, true);
    add("Flow", "Flow", false, true);
    add("Frost", "Frost", false, true);
    return out;
}

bool MaterialPropertyGraph::parse(const QString& json, MaterialPropertyGraph& out)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError)
    {
        LOG_WARNING(QString("MaterialPropertyGraph: JSON error: %1").arg(err.errorString()));
        return false;
    }

    QJsonArray models;
    if (doc.isArray())
    {
        models = doc.array();
    }
    else if (doc.isObject())
    {
        const QJsonObject root = doc.object();
        out.name = root.value(QStringLiteral("name")).toString();
        if (root.contains(QStringLiteral("shaderModels")))
            models = root.value(QStringLiteral("shaderModels")).toArray();
        else if (root.contains(QStringLiteral("models")))
            models = root.value(QStringLiteral("models")).toArray();
        else
            return false;
    }
    else
    {
        return false;
    }

    for (const QJsonValue& mv : models)
    {
        if (!mv.isObject())
            continue;
        const QJsonObject mobj = mv.toObject();
        MaterialShaderModel model;
        model.name = mobj.value(QStringLiteral("name")).toString();
        model.displayName = mobj.value(QStringLiteral("displayName")).toString(
            mobj.value(QStringLiteral("label")).toString());
        if (model.displayName.isEmpty())
            model.displayName = model.name;
        model.blenderCount = mobj.value(QStringLiteral("blenders")).toInt(
            mobj.value(QStringLiteral("blenderCount")).toInt(0));
        model.hasSubsurfaceScattering =
            mobj.value(QStringLiteral("subsurfaceScattering")).toBool(false);
        model.hasTranslucency = mobj.value(QStringLiteral("translucency")).toBool(false);
        model.isWater = mobj.value(QStringLiteral("water")).toBool(false);
        model.isSkin = mobj.value(QStringLiteral("skin")).toBool(false);

        const QJsonValue slotsVal = mobj.value(QStringLiteral("textureSlots"));
        if (slotsVal.isArray())
        {
            for (const QJsonValue& sv : slotsVal.toArray())
            {
                if (!sv.isObject())
                    continue;
                const QJsonObject sObj = sv.toObject();
                MaterialTextureSlot slot;
                slot.name = sObj.value(QStringLiteral("name")).toString();
                slot.textureKey = sObj.value(QStringLiteral("key")).toString(
                    sObj.value(QStringLiteral("textureKey")).toString());
                slot.mandatory = sObj.value(QStringLiteral("mandatory")).toBool(false);
                slot.optional = sObj.value(QStringLiteral("optional")).toBool(false);
                model.textureSlots.append(slot);
            }
        }
        else if (slotsVal.isString())
        {
            // A comma-separated list of standard slot names.
            const QStringList names = slotsVal.toString().split(',', Qt::SkipEmptyParts);
            const QVector<MaterialTextureSlot> standard = standardSlots();
            for (const QString& n : names)
            {
                const QString trimmed = n.trimmed();
                for (const MaterialTextureSlot& s : standard)
                    if (s.name.compare(trimmed, Qt::CaseInsensitive) == 0)
                        model.textureSlots.append(s);
            }
        }

        out.models.append(model);
    }

    if (out.commonSlots.isEmpty())
        out.commonSlots = standardSlots();
    return !out.models.isEmpty();
}

bool MaterialPropertyGraph::loadFile(const QString& path, MaterialPropertyGraph& out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_WARNING(QString("MaterialPropertyGraph: cannot open %1").arg(path));
        return false;
    }
    const QString json = QString::fromUtf8(file.readAll());
    return parse(json, out);
}

const MaterialShaderModel* MaterialPropertyGraph::findModel(const QString& name) const
{
    for (const MaterialShaderModel& model : models)
        if (model.name.compare(name, Qt::CaseInsensitive) == 0)
            return &model;
    return nullptr;
}

MaterialPropertyGraph MaterialPropertyGraph::builtin()
{
    MaterialPropertyGraph graph;
    graph.name = QStringLiteral("Default");
    graph.commonSlots = standardSlots();

    const auto model = [&](const QString& name, const char* display,
                           int blenders, bool water = false, bool skin = false,
                           bool sss = false, bool translucency = false) {
        MaterialShaderModel m;
        m.name = name;
        m.displayName = QString::fromLatin1(display);
        m.blenderCount = blenders;
        m.isWater = water;
        m.isSkin = skin;
        m.hasSubsurfaceScattering = sss;
        m.hasTranslucency = translucency;
        m.textureSlots = standardSlots();
        return m;
    };

    graph.models.append(model("1LayerStandard", "1 Layer Standard", 1));
    graph.models.append(model("2LayerStandard", "2 Layer Standard", 2));
    graph.models.append(model("3LayerStandard", "3 Layer Standard", 3));
    graph.models.append(model("4LayerStandard", "4 Layer Standard", 4));
    graph.models.append(model("Terrain", "Terrain", 1));
    graph.models.append(model("Skin", "Skin", 0, false, true, true, true));
    graph.models.append(model("Hair", "Hair", 0, false, true, true, true));
    graph.models.append(model("Eye", "Eye", 0, false, false, true, true));
    graph.models.append(model("Water", "Water", 0, true, false, true, true));
    graph.models.append(model("Vegetation", "Vegetation", 1));

    return graph;
}
