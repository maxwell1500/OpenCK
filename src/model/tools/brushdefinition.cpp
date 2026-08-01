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
    if (ok) *ok = false;
    return Operation::Sculpt;
}

BrushDefinition BrushDefinition::fromJson(const QJsonObject& obj)
{
    BrushDefinition b;
    b.name = obj.value(QStringLiteral("name")).toString().trimmed();
    if (b.name.isEmpty()) {
        b.name = obj.value(QStringLiteral("Name")).toString().trimmed();
    }

    bool ok = false;
    const QString opText = obj.value(QStringLiteral("operation")).toString();
    if (opText.isEmpty()) {
        b.operation = stringToOperation(obj.value(QStringLiteral("Operation")).toString(), &ok);
    } else {
        b.operation = stringToOperation(opText, &ok);
    }
    if (!ok) {
        // A brush with an unknown operation type is skipped by the caller
        // unless it has an explicit operation; default to Sculpt and let the
        // loader caller decide whether to keep it.
        b.operation = Operation::Sculpt;
    }

    b.radius = obj.value(QStringLiteral("radius")).toDouble(obj.value(QStringLiteral("Radius")).toDouble(b.radius));
    b.strength = obj.value(QStringLiteral("strength")).toDouble(obj.value(QStringLiteral("Strength")).toDouble(b.strength));
    b.falloff = obj.value(QStringLiteral("falloff")).toDouble(obj.value(QStringLiteral("Falloff")).toDouble(b.falloff));
    b.invert = obj.value(QStringLiteral("invert")).toBool(obj.value(QStringLiteral("Invert")).toBool(b.invert));
    b.targetHeight = obj.value(QStringLiteral("targetHeight")).toDouble(obj.value(QStringLiteral("TargetHeight")).toDouble(b.targetHeight));
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

    QJsonArray arr;
    if (doc.isArray()) {
        arr = doc.array();
    } else if (doc.isObject()) {
        arr = doc.object().value(QStringLiteral("brushes")).toArray();
    }

    int count = 0;
    for (const QJsonValue& v : arr) {
        if (!v.isObject()) continue;
        const BrushDefinition b = fromJson(v.toObject());
        if (b.name.isEmpty()) continue;
        out.append(b);
        ++count;
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

    return brushes;
}
