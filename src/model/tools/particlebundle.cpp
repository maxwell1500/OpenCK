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
