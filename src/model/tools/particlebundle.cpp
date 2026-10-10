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

static double toDoubleSafe(const QJsonValue& v, double def = 0.0)
{
    if (v.isDouble())
        return v.toDouble();
    if (v.isString())
    {
        bool ok = false;
        const double d = v.toString().toDouble(&ok);
        if (ok) return d;
    }
    return def;
}

static bool parseStarfieldPofx(const QJsonObject& rootObj, ParticleBundle& out)
{
    const QJsonObject dataObj = rootObj.value(QStringLiteral("Data")).toObject();
    if (dataObj.isEmpty())
        return false;

    QString bundleName;
    const QJsonValue bundlesVal = dataObj.value(QStringLiteral("Bundles"));
    if (bundlesVal.isObject())
    {
        const QJsonValue bData = bundlesVal.toObject().value(QStringLiteral("Data"));
        if (bData.isArray())
        {
            for (const QJsonValue& bItem : bData.toArray())
            {
                if (bItem.isObject())
                {
                    const QJsonObject innerData = bItem.toObject().value(QStringLiteral("Data")).toObject();
                    const QString dname = innerData.value(QStringLiteral("DisplayName")).toString();
                    if (!dname.isEmpty())
                    {
                        bundleName = dname;
                        break;
                    }
                }
            }
        }
    }

    QJsonObject defObj;
    const QJsonValue defVal = dataObj.value(QStringLiteral("Definition"));
    if (defVal.isObject())
    {
        defObj = defVal.toObject().value(QStringLiteral("Data")).toObject();
    }
    if (defObj.isEmpty())
        defObj = dataObj;

    ParticleBundle::Node node;
    node.name = bundleName.isEmpty() ? QStringLiteral("ParticleNode") : bundleName;
    node.bundle = bundleName;

    // Material / texture
    QString matName;
    const QJsonObject matObj = defObj.value(QStringLiteral("Material")).toObject().value(QStringLiteral("Data")).toObject();
    if (!matObj.isEmpty())
    {
        matName = matObj.value(QStringLiteral("Name")).toString();
    }
    if (matName.isEmpty() || matName == QStringLiteral("None"))
    {
        const QJsonObject cloudMat = defObj.value(QStringLiteral("Cloud")).toObject()
            .value(QStringLiteral("Data")).toObject()
            .value(QStringLiteral("BaseMaterial")).toObject()
            .value(QStringLiteral("Data")).toObject();
        matName = cloudMat.value(QStringLiteral("Name")).toString();
    }
    node.texture = matName;

    // Gravity
    const QJsonObject gravObj = defObj.value(QStringLiteral("Gravity")).toObject().value(QStringLiteral("Data")).toObject();
    if (!gravObj.isEmpty())
    {
        const float gx = static_cast<float>(toDoubleSafe(gravObj.value(QStringLiteral("x")), 0.0));
        const float gy = static_cast<float>(toDoubleSafe(gravObj.value(QStringLiteral("y")), 0.0));
        const float gz = static_cast<float>(toDoubleSafe(gravObj.value(QStringLiteral("z")), 0.0));
        node.gravity = std::sqrt(gx * gx + gy * gy + gz * gz);
    }

    // Velocity / Speed
    if (defObj.contains(QStringLiteral("Velocity")))
    {
        const QJsonObject velObj = defObj.value(QStringLiteral("Velocity")).toObject().value(QStringLiteral("Data")).toObject();
        if (!velObj.isEmpty())
        {
            const float vx = static_cast<float>(toDoubleSafe(velObj.value(QStringLiteral("x")), 0.0));
            const float vy = static_cast<float>(toDoubleSafe(velObj.value(QStringLiteral("y")), 0.0));
            const float vz = static_cast<float>(toDoubleSafe(velObj.value(QStringLiteral("z")), 0.0));
            node.velocity = std::sqrt(vx * vx + vy * vy + vz * vz);
        }
        else
        {
            node.velocity = static_cast<float>(toDoubleSafe(defObj.value(QStringLiteral("Velocity")), 0.0));
        }
    }

    // Lifetime / Age
    if (defObj.contains(QStringLiteral("Age")))
        node.age = static_cast<float>(toDoubleSafe(defObj.value(QStringLiteral("Age")), 1.0));
    else if (defObj.contains(QStringLiteral("Lifetime")))
        node.age = static_cast<float>(toDoubleSafe(defObj.value(QStringLiteral("Lifetime")), 1.0));

    // Ribbon
    const QString ptype = defObj.value(QStringLiteral("ParticleType")).toString();
    if (ptype.compare(QStringLiteral("Ribbon"), Qt::CaseInsensitive) == 0 || defObj.contains(QStringLiteral("RibbonUVCurve")))
    {
        node.ribbon = true;
    }

    // UV Scroll
    if (matObj.value(QStringLiteral("Type")).toString().contains(QStringLiteral("AnimatedUV"), Qt::CaseInsensitive))
    {
        node.uvScroll = true;
    }

    // FlipBook
    const QJsonObject cloudObj = defObj.value(QStringLiteral("Cloud")).toObject().value(QStringLiteral("Data")).toObject();
    const QJsonObject baseMat = cloudObj.value(QStringLiteral("BaseMaterial")).toObject().value(QStringLiteral("Data")).toObject();
    if (baseMat.value(QStringLiteral("FlipbooksTimeBased")).toString().compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0)
    {
        node.flipBook.loop = true;
    }

    // Scan Stacks for Simulation definitions (Gravity, Drag, Attractor, Turbulence, etc.)
    const QJsonObject stacksObj = defObj.value(QStringLiteral("Stacks")).toObject().value(QStringLiteral("Data")).toObject();
    const QJsonArray simDefs = stacksObj.value(QStringLiteral("SimulationDefinitionA")).toObject().value(QStringLiteral("Data")).toArray();
    for (const QJsonValue& sv : simDefs)
    {
        if (!sv.isObject()) continue;
        const QJsonObject sdata = sv.toObject().value(QStringLiteral("Data")).toObject().value(QStringLiteral("Data")).toObject();
        const QString displayName = sdata.value(QStringLiteral("DisplayName")).toString();

        if (displayName.contains(QStringLiteral("Gravity"), Qt::CaseInsensitive) && node.gravity == 0.0f)
        {
            const QJsonObject pset = sdata.value(QStringLiteral("Intrinsic")).toObject()
                .value(QStringLiteral("Data")).toObject()
                .value(QStringLiteral("upParameterSet")).toObject()
                .value(QStringLiteral("Data")).toObject()
                .value(QStringLiteral("Data")).toObject();
            const QJsonObject constObj = pset.value(QStringLiteral("A")).toObject()
                .value(QStringLiteral("Data")).toObject()
                .value(QStringLiteral("Constant")).toObject()
                .value(QStringLiteral("Data")).toObject();
            const float z = static_cast<float>(toDoubleSafe(constObj.value(QStringLiteral("z")), 0.0));
            node.gravity = std::abs(z);
        }
        else if (displayName.contains(QStringLiteral("Drag"), Qt::CaseInsensitive))
        {
            node.drag = 1.0f;
        }
        else if (displayName.contains(QStringLiteral("Attractor"), Qt::CaseInsensitive))
        {
            ParticleBundle::Node::Attractor attr;
            attr.name = displayName;
            attr.strength = 1.0f;
            node.attractors.append(attr);
        }
        else if (displayName.contains(QStringLiteral("Turbulence"), Qt::CaseInsensitive))
        {
            node.turbulence.strength = 1.0f;
        }
        else if (displayName.contains(QStringLiteral("Flipbook"), Qt::CaseInsensitive))
        {
            node.flipBook.loop = true;
        }
    }

    if (bundleName.contains(QStringLiteral("Attractor"), Qt::CaseInsensitive) && node.attractors.isEmpty())
    {
        ParticleBundle::Node::Attractor attr;
        attr.name = bundleName;
        attr.strength = 1.0f;
        node.attractors.append(attr);
    }
    if (bundleName.contains(QStringLiteral("Turbulence"), Qt::CaseInsensitive) && node.turbulence.strength == 0.0f)
    {
        node.turbulence.strength = 1.0f;
    }
    if (bundleName.contains(QStringLiteral("Flipbook"), Qt::CaseInsensitive))
    {
        node.flipBook.loop = true;
    }

    out.name = bundleName;
    out.nodes.append(node);
    return true;
}

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

    if (doc.isObject() && doc.object().contains(QStringLiteral("Data")))
    {
        if (parseStarfieldPofx(doc.object(), bundle))
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
    if (out.name.isEmpty())
    {
        out.name = QFileInfo(path).baseName();
    }
    LOG_DEBUG(QString("ParticleBundle: parsed %1 nodes from %2")
        .arg(out.nodes.size()).arg(path));
    return true;
}
