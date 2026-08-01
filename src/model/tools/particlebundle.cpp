#include "particlebundle.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

#include "../../files/log/logger.hpp"

namespace {

void parseNode(const QJsonObject& obj, const QString& bundleName, ParticleBundle& out)
{
    ParticleBundle::Node node;
    node.name = obj.value(QStringLiteral("name")).toString();
    node.bundle = bundleName;
    node.age = static_cast<float>(obj.value(QStringLiteral("age")).toDouble(
        obj.value(QStringLiteral("lifetime")).toDouble(1.0)));
    node.alphaByCurve = static_cast<float>(obj.value(QStringLiteral("alphaByCurve")).toDouble(
        obj.value(QStringLiteral("alpha")).toDouble(1.0)));
    node.velocity = static_cast<float>(obj.value(QStringLiteral("velocity")).toDouble(0.0));
    node.gravity = static_cast<float>(obj.value(QStringLiteral("gravity")).toDouble(0.0));
    node.drag = static_cast<float>(obj.value(QStringLiteral("drag")).toDouble(0.0));
    node.rotationSpeed = static_cast<float>(obj.value(QStringLiteral("rotationSpeed")).toDouble(
        obj.value(QStringLiteral("rotation")).toDouble(0.0)));
    node.ribbon = obj.value(QStringLiteral("ribbon")).toBool(false);
    node.uvScroll = obj.value(QStringLiteral("uvScroll")).toBool(false);
    node.texture = obj.value(QStringLiteral("texture")).toString();
    if (node.name.isEmpty())
        node.name = QStringLiteral("Unnamed");

    // Attractors: an array of {name, x, y, z, strength, radius}.
    const QJsonValue attrArr = obj.value(QStringLiteral("attractors"));
    if (attrArr.isArray())
    {
        for (const QJsonValue& av : attrArr.toArray())
        {
            if (!av.isObject())
                continue;
            const QJsonObject aobj = av.toObject();
            ParticleBundle::Node::Attractor attr;
            attr.name = aobj.value(QStringLiteral("name")).toString();
            attr.x = static_cast<float>(aobj.value(QStringLiteral("x")).toDouble(0.0));
            attr.y = static_cast<float>(aobj.value(QStringLiteral("y")).toDouble(0.0));
            attr.z = static_cast<float>(aobj.value(QStringLiteral("z")).toDouble(0.0));
            attr.strength = static_cast<float>(aobj.value(QStringLiteral("strength")).toDouble(1.0));
            attr.radius = static_cast<float>(aobj.value(QStringLiteral("radius")).toDouble(1.0));
            node.attractors.append(attr);
        }
    }

    // Turbulence: {strength, frequency}.
    const QJsonValue turbVal = obj.value(QStringLiteral("turbulence"));
    if (turbVal.isObject())
    {
        const QJsonObject tobj = turbVal.toObject();
        node.turbulence.strength = static_cast<float>(tobj.value(QStringLiteral("strength")).toDouble(0.0));
        node.turbulence.frequency = static_cast<float>(tobj.value(QStringLiteral("frequency")).toDouble(1.0));
    }

    // FlipBook: {columns, rows, frameRate, loop}.
    const QJsonValue fbVal = obj.value(QStringLiteral("flipBook"));
    if (fbVal.isObject())
    {
        const QJsonObject fobj = fbVal.toObject();
        node.flipBook.columns = fobj.value(QStringLiteral("columns")).toInt(
            fobj.value(QStringLiteral("cols")).toInt(1));
        node.flipBook.rows = fobj.value(QStringLiteral("rows")).toInt(1);
        node.flipBook.frameRate = static_cast<float>(fobj.value(QStringLiteral("frameRate")).toDouble(30.0));
        node.flipBook.loop = fobj.value(QStringLiteral("loop")).toBool(false);
    }

    out.nodes.append(node);
}

} // namespace

ParticleBundle ParticleBundle::parse(const QByteArray& json)
{
    ParticleBundle bundle;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError)
    {
        LOG_WARNING(QString("ParticleBundle: JSON error: %1").arg(err.errorString()));
        return bundle;
    }

    QJsonArray bundles;
    if (doc.isArray())
    {
        bundles = doc.array();
    }
    else if (doc.isObject())
    {
        bundles.append(doc.object());
    }

    for (const QJsonValue& bv : bundles)
    {
        if (!bv.isObject()) continue;
        const QJsonObject bobj = bv.toObject();

        QString bundleName = bobj.value(QStringLiteral("name")).toString();
        if (bundleName.isEmpty())
            bundleName = bobj.value(QStringLiteral("bundle")).toString();
        if (bundle.name.isEmpty())
            bundle.name = bundleName;

        // Nested node arrays under "nodes" or "emitters".
        bool found = false;
        for (const char* key : { "nodes", "emitters" })
        {
            const QJsonValue arr = bobj.value(QLatin1String(key));
            if (!arr.isArray()) continue;
            for (const QJsonValue& nv : arr.toArray())
            {
                if (nv.isObject())
                {
                    parseNode(nv.toObject(), bundleName, bundle);
                    found = true;
                }
            }
        }
        // A bundle object that is itself a node (single-node bundle).
        if (!found && !bundleName.isEmpty())
        {
            parseNode(bobj, bundleName, bundle);
        }
    }
    return bundle;
}

bool ParticleBundle::loadFile(const QString& path, ParticleBundle& out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_WARNING(QString("ParticleBundle::loadFile: cannot open %1").arg(path));
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();

    out = parse(data);
    LOG_DEBUG(QString("ParticleBundle: parsed %1 nodes from %2")
        .arg(out.nodes.size()).arg(path));
    return true;
}
