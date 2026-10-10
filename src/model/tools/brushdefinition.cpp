#include "brushdefinition.hpp"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QFile>

#include "../../files/log/logger.hpp"

QString BrushDefinition::operationToString(Operation op)
{
    switch (op)
    {
    case Operation::Sculpt: return QStringLiteral("Sculpt");
    case Operation::Flatten: return QStringLiteral("Flatten");
    case Operation::Smooth: return QStringLiteral("Smooth");
    case Operation::Stamp: return QStringLiteral("Stamp");
    case Operation::BuildUp: return QStringLiteral("BuildUp");
    case Operation::Subtractive: return QStringLiteral("Subtractive");
    case Operation::Noise: return QStringLiteral("Noise");
    }
    return QStringLiteral("Sculpt");
}

BrushDefinition::Operation BrushDefinition::stringToOperation(const QString& text, bool* ok)
{
    const QString t = text.trimmed();
    if (t.compare(QStringLiteral("Sculpt"), Qt::CaseInsensitive) == 0) { if (ok) *ok = true; return Operation::Sculpt; }
    if (t.compare(QStringLiteral("Flatten"), Qt::CaseInsensitive) == 0) { if (ok) *ok = true; return Operation::Flatten; }
    if (t.compare(QStringLiteral("Smooth"), Qt::CaseInsensitive) == 0) { if (ok) *ok = true; return Operation::Smooth; }
    if (t.compare(QStringLiteral("Stamp"), Qt::CaseInsensitive) == 0) { if (ok) *ok = true; return Operation::Stamp; }
    if (t.compare(QStringLiteral("BuildUp"), Qt::CaseInsensitive) == 0) { if (ok) *ok = true; return Operation::BuildUp; }
    if (t.compare(QStringLiteral("Subtractive"), Qt::CaseInsensitive) == 0) { if (ok) *ok = true; return Operation::Subtractive; }
    if (t.compare(QStringLiteral("Noise"), Qt::CaseInsensitive) == 0) { if (ok) *ok = true; return Operation::Noise; }
    if (ok) *ok = false;
    return Operation::Sculpt;
}

BrushDefinition BrushDefinition::fromJson(const QJsonObject& obj, const QString& fallbackName)
{
    BrushDefinition b;
    b.name = obj.value(QStringLiteral("name")).toString().trimmed();
    if (b.name.isEmpty()) {
        b.name = obj.value(QStringLiteral("Name")).toString().trimmed();
    }
    if (b.name.isEmpty() && !fallbackName.isEmpty()) {
        b.name = fallbackName;
    }

    bool ok = false;
    const QString opText = obj.value(QStringLiteral("operation")).toString();
    if (!opText.isEmpty()) {
        b.operation = stringToOperation(opText, &ok);
    } else if (obj.contains(QStringLiteral("Operation"))) {
        b.operation = stringToOperation(obj.value(QStringLiteral("Operation")).toString(), &ok);
    }

    if (!ok) {
        // Starfield boolean flags schema
        if (obj.value(QStringLiteral("Flatten")).toBool(false)) {
            b.operation = Operation::Flatten;
            ok = true;
        } else if (obj.value(QStringLiteral("Smooth")).toBool(false)) {
            b.operation = Operation::Smooth;
            ok = true;
        } else if (obj.value(QStringLiteral("StampMode")).toBool(false)) {
            b.operation = Operation::Stamp;
            ok = true;
        } else if (obj.value(QStringLiteral("BuildUp")).toBool(false)) {
            b.operation = Operation::BuildUp;
            ok = true;
        } else if (obj.value(QStringLiteral("Subtractive")).toBool(false)) {
            b.operation = Operation::Subtractive;
            ok = true;
        } else if (obj.value(QStringLiteral("Noise")).toBool(false)) {
            b.operation = Operation::Noise;
            ok = true;
        } else if (obj.value(QStringLiteral("Sculpt")).toBool(false)) {
            ok = true;
        } else {
            b.operation = Operation::Sculpt;
        }
    }

    // Radius / Size
    if (obj.contains(QStringLiteral("radius"))) {
        b.radius = obj.value(QStringLiteral("radius")).toDouble(b.radius);
    } else if (obj.contains(QStringLiteral("Radius"))) {
        b.radius = obj.value(QStringLiteral("Radius")).toDouble(b.radius);
    } else if (obj.contains(QStringLiteral("Size"))) {
        b.radius = obj.value(QStringLiteral("Size")).toDouble(b.radius);
    }

    // Strength
    if (obj.contains(QStringLiteral("strength"))) {
        b.strength = obj.value(QStringLiteral("strength")).toDouble(b.strength);
    } else if (obj.contains(QStringLiteral("Strength"))) {
        b.strength = obj.value(QStringLiteral("Strength")).toDouble(b.strength);
    }

    // Falloff / FalloffProfile
    if (obj.contains(QStringLiteral("falloff"))) {
        b.falloff = obj.value(QStringLiteral("falloff")).toDouble(b.falloff);
    } else if (obj.contains(QStringLiteral("Falloff"))) {
        b.falloff = obj.value(QStringLiteral("Falloff")).toDouble(b.falloff);
    } else if (obj.contains(QStringLiteral("FalloffProfile"))) {
        b.falloff = obj.value(QStringLiteral("FalloffProfile")).toDouble(b.falloff);
    }

    // Invert
    if (obj.contains(QStringLiteral("invert"))) {
        b.invert = obj.value(QStringLiteral("invert")).toBool(b.invert);
    } else if (obj.contains(QStringLiteral("Invert"))) {
        b.invert = obj.value(QStringLiteral("Invert")).toBool(b.invert);
    } else if (obj.contains(QStringLiteral("InvertSlopeInfluence"))) {
        b.invert = obj.value(QStringLiteral("InvertSlopeInfluence")).toBool(b.invert);
    }

    // TargetHeight / SculptHeight
    if (obj.contains(QStringLiteral("targetHeight"))) {
        b.targetHeight = obj.value(QStringLiteral("targetHeight")).toDouble(b.targetHeight);
    } else if (obj.contains(QStringLiteral("TargetHeight"))) {
        b.targetHeight = obj.value(QStringLiteral("TargetHeight")).toDouble(b.targetHeight);
    } else if (obj.contains(QStringLiteral("SculptHeight"))) {
        b.targetHeight = obj.value(QStringLiteral("SculptHeight")).toDouble(b.targetHeight);
    }

    // Alpha mask
    if (obj.contains(QStringLiteral("Alpha"))) {
        b.alphaMask = obj.value(QStringLiteral("Alpha")).toString();
    } else if (obj.contains(QStringLiteral("alpha"))) {
        b.alphaMask = obj.value(QStringLiteral("alpha")).toString();
    }

    return b;
}

bool BrushDefinition::loadFile(const QString& path, QVector<BrushDefinition>& out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_WARNING(QString("BrushDefinition::loadFile: cannot open %1").arg(path));
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        LOG_WARNING(QString("BrushDefinition::loadFile: JSON error in %1: %2")
            .arg(path).arg(err.errorString()));
        return false;
    }

    const QString fallbackName = QFileInfo(path).baseName();
    int count = 0;
    if (doc.isArray()) {
        const QJsonArray arr = doc.array();
        for (const QJsonValue& v : arr) {
            if (!v.isObject()) continue;
            const BrushDefinition b = fromJson(v.toObject());
            if (b.name.isEmpty()) continue;
            out.append(b);
            ++count;
        }
    } else if (doc.isObject()) {
        const QJsonObject root = doc.object();
        if (root.contains(QStringLiteral("brushes")) && root.value(QStringLiteral("brushes")).isArray()) {
            const QJsonArray arr = root.value(QStringLiteral("brushes")).toArray();
            for (const QJsonValue& v : arr) {
                if (!v.isObject()) continue;
                const BrushDefinition b = fromJson(v.toObject());
                if (b.name.isEmpty()) continue;
                out.append(b);
                ++count;
            }
        } else {
            // Single brush object format (.lbr as shipped by Bethesda Creation Kit)
            const BrushDefinition b = fromJson(root, fallbackName);
            if (!b.name.isEmpty()) {
                out.append(b);
                ++count;
            }
        }
    }
    LOG_DEBUG(QString("BrushDefinition::loadFile: loaded %1 brushes from %2").arg(count).arg(path));
    return count > 0;
}

QVector<BrushDefinition> BrushDefinition::builtin()
{
    QVector<BrushDefinition> brushes;

    BrushDefinition sculpt;
    sculpt.name = QStringLiteral("Sculpt");
    sculpt.operation = Operation::Sculpt;
    sculpt.radius = 5.0;
    sculpt.strength = 10.0;
    sculpt.falloff = 0.5;
    brushes.append(sculpt);

    BrushDefinition flatten;
    flatten.name = QStringLiteral("Flatten");
    flatten.operation = Operation::Flatten;
    flatten.radius = 5.0;
    flatten.strength = 20.0;
    flatten.falloff = 0.3;
    brushes.append(flatten);

    BrushDefinition smooth;
    smooth.name = QStringLiteral("Smooth");
    smooth.operation = Operation::Smooth;
    smooth.radius = 5.0;
    smooth.strength = 10.0;
    smooth.falloff = 0.5;
    brushes.append(smooth);

    BrushDefinition stamp;
    stamp.name = QStringLiteral("Stamp");
    stamp.operation = Operation::Stamp;
    stamp.radius = 6.0;
    stamp.strength = 10.0;
    stamp.falloff = 0.4;
    brushes.append(stamp);

    BrushDefinition buildUp;
    buildUp.name = QStringLiteral("BuildUp");
    buildUp.operation = Operation::BuildUp;
    buildUp.radius = 5.0;
    buildUp.strength = 12.0;
    buildUp.falloff = 0.6;
    brushes.append(buildUp);

    BrushDefinition subtractive;
    subtractive.name = QStringLiteral("Subtractive");
    subtractive.operation = Operation::Subtractive;
    subtractive.radius = 5.0;
    subtractive.strength = 12.0;
    subtractive.falloff = 0.6;
    brushes.append(subtractive);

    BrushDefinition noise;
    noise.name = QStringLiteral("Noise");
    noise.operation = Operation::Noise;
    noise.radius = 5.0;
    noise.strength = 8.0;
    noise.falloff = 0.5;
    brushes.append(noise);

    return brushes;
}
