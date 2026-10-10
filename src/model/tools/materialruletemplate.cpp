#include "materialruletemplate.hpp"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>

#include "../../files/log/logger.hpp"

namespace
{

QStringList splitPath(const QString& path)
{
    QStringList segs = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    return segs;
}

// The TextureSet/<slot> name in a CK property path, e.g.
// "Layer1/Material/TextureSet/Diffuse" -> "Diffuse". Empty when the path
// does not address a texture slot.
QString textureSlotOf(const QString& path)
{
    const QStringList segs = splitPath(path);
    for (int i = 0; i + 1 < segs.size(); ++i)
    {
        if (segs[i].compare(QLatin1String("TextureSet"), Qt::CaseInsensitive) == 0)
        {
            return segs[i + 1];
        }
    }
    return QString();
}

bool isLayerGroup(const QString& path)
{
    // A single-segment "LayerN" (or "Layer*") group path.
    static const QRegularExpression layerRe(QRegularExpression::anchoredPattern(
        QStringLiteral("^Layer(?:\\d+|\\*)+$")));
    const QStringList segs = splitPath(path);
    return segs.size() == 1 && layerRe.match(segs[0]).hasMatch();
}

} // namespace

MaterialRuleTemplate MaterialRuleTemplate::fromJson(const QJsonObject& obj)
{
    MaterialRuleTemplate tpl;
    tpl.category = obj.value(QStringLiteral("Category")).toString();
    tpl.name = obj.value(QStringLiteral("Name")).toString(
        obj.value(QStringLiteral("name")).toString());

    const QJsonValue meta = obj.value(QStringLiteral("MetaData"));
    if (meta.isObject())
    {
        const QJsonObject mo = meta.toObject();
        tpl.rootMaterial = mo.value(QStringLiteral("RootMaterial")).toString();
        tpl.displayName = mo.value(QStringLiteral("DisplayName")).toString();
        tpl.complexityCost = mo.value(QStringLiteral("ComplexityCost")).toInt(0);
    }
    else if (meta.isString())
    {
        // Bundles ship a plain string (e.g. "Nothing").
        tpl.displayName = meta.toString();
    }

    tpl.version = obj.value(QStringLiteral("Version")).toString();

    const QJsonValue rules = obj.value(QStringLiteral("TemplateRules"));
    if (rules.isArray())
    {
        for (const QJsonValue& cv : rules.toArray())
        {
            if (!cv.isObject()) continue;
            const QJsonObject co = cv.toObject();
            RuleClass rc;
            rc.className = co.value(QStringLiteral("Class")).toString();

            const QJsonValue rr = co.value(QStringLiteral("Rules"));
            if (rr.isArray())
            {
                for (const QJsonValue& rv : rr.toArray())
                {
                    if (!rv.isObject()) continue;
                    const QJsonObject ro = rv.toObject();
                    Rule rule;
                    rule.from = ro.value(QStringLiteral("From")).toString();
                    rule.to = ro.value(QStringLiteral("To")).toString();
                    rule.op = ro.value(QStringLiteral("Op")).toString();
                    if (!rule.op.isEmpty() && !rule.from.isEmpty())
                        rc.rules.append(rule);
                }
            }
            if (!rc.rules.isEmpty())
                tpl.ruleClasses.append(rc);
        }
    }
    return tpl;
}

bool MaterialRuleTemplate::loadFile(const QString& path, MaterialRuleTemplate& out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_WARNING(QString("MaterialRuleTemplate::loadFile: cannot open %1").arg(path));
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError)
    {
        LOG_WARNING(QString("MaterialRuleTemplate: JSON error in %1: %2")
            .arg(path).arg(err.errorString()));
        return false;
    }

    QJsonObject obj;
    if (doc.isObject())
    {
        obj = doc.object();
    }
    else if (doc.isArray() && !doc.array().isEmpty())
    {
        obj = doc.array().first().toObject();
    }
    else
    {
        LOG_WARNING(QString("MaterialRuleTemplate: %1 is neither an object nor an array").arg(path));
        return false;
    }

    out = fromJson(obj);
    LOG_DEBUG(QString("MaterialRuleTemplate: parsed '%1' from %2")
        .arg(out.name).arg(path));
    return !out.name.isEmpty();
}

int MaterialRuleTemplate::loadDirectory(const QString& dir, QVector<MaterialRuleTemplate>& out)
{
    QDir d(dir);
    if (!d.exists())
    {
        LOG_DEBUG(QString("MaterialRuleTemplate: directory not found: %1").arg(dir));
        return 0;
    }

    const QStringList files = d.entryList(QDir::Files, QDir::Name);
    int loaded = 0;
    for (const QString& f : files)
    {
        if (!f.endsWith(QLatin1String(".json"), Qt::CaseInsensitive)) continue;
        MaterialRuleTemplate tpl;
        if (loadFile(d.absoluteFilePath(f), tpl))
        {
            out.append(tpl);
            ++loaded;
        }
    }
    LOG_DEBUG(QString("MaterialRuleTemplate: loaded %1 templates from %2")
        .arg(loaded).arg(dir));
    return loaded;
}

QStringList MaterialRuleTemplate::builtinLayerSlots()
{
    // The standard texture slots one CK material layer can carry.
    return {
        QStringLiteral("Diffuse"),
        QStringLiteral("Normal"),
        QStringLiteral("Roughness"),
        QStringLiteral("Metalness"),
        QStringLiteral("AO"),
        QStringLiteral("Curvature"),
        QStringLiteral("Emissive"),
        QStringLiteral("Mask"),
        QStringLiteral("Height"),
        QStringLiteral("Flow"),
        QStringLiteral("Transmissive"),
        QStringLiteral("Frost"),
        QStringLiteral("HairID"),
        QStringLiteral("Secondary Opacity"),
        QStringLiteral("DepthOffset"),
        QStringLiteral("ScatteringDirections"),
        QStringLiteral("ScatteringForwardAlpha"),
        QStringLiteral("Overlay Color"),
        QStringLiteral("Overlay Roughness"),
        QStringLiteral("Overlay Metalness")
    };
}

QRegularExpression MaterialRuleTemplate::patternToRegex(const QString& pattern)
{
    if (pattern.isEmpty())
        return QRegularExpression(QRegularExpression::anchoredPattern(QStringLiteral("(?!x)x")));
    if (pattern == QLatin1String("*"))
        return QRegularExpression(QRegularExpression::anchoredPattern(QStringLiteral(".*")));

    const QStringList segs = splitPath(pattern);
    QStringList parts;
    for (int i = 0; i < segs.size(); ++i)
    {
        const QString seg = segs[i];
        QString piece;
        if (seg == QLatin1String("*"))
        {
            // A bare '*' segment: last segment matches any remaining depth,
            // an inner '*' segment matches within that segment only.
            piece = (i + 1 == segs.size())
                ? QStringLiteral(".*")
                : QStringLiteral("[^/]*");
        }
        else
        {
            // Escape regex metacharacters, then un-escape the wildcards.
            piece = QRegularExpression::escape(seg);
            piece.replace(QLatin1String("\\*"), QLatin1String("[^/]*"));
        }
        parts.append(piece);
    }
    return QRegularExpression(QRegularExpression::anchoredPattern(parts.join(QLatin1Char('/'))));
}

bool MaterialRuleTemplate::patternMatches(const QString& pattern, const QString& path)
{
    return patternToRegex(pattern).match(path).hasMatch();
}

QStringList MaterialRuleTemplate::textureSlots() const
{
    QStringList slotList;
    for (const RuleClass& rc : ruleClasses)
    {
        for (const Rule& r : rc.rules)
        {
            // Only the "From" slot is part of the material's texture set; the
            // "To" of a Move rule is a destination the template routes to, not
            // a slot the material declares.
            const QString slot = textureSlotOf(r.from);
            if (slot.isEmpty() || slot.contains(QLatin1Char('*'))) continue;
            if (!slotList.contains(slot))
                slotList.append(slot);
        }
    }
    return slotList;
}

MaterialRuleTemplate::ApplyMapResult
MaterialRuleTemplate::applyToSlotMap(const QMap<QString, QString>& slotMap) const
{
    ApplyMapResult res;
    res.slotPaths = slotMap;

    auto log = [&](const Rule& r, const QString& slot)
    {
        res.operations << (slot.isEmpty() ? r.op + QLatin1Char(':') + r.from
                                          : r.op + QLatin1Char(':') + slot);
    };

    for (const RuleClass& rc : ruleClasses)
    {
        for (const Rule& r : rc.rules)
        {
            const QString op = r.op.toUpper();

            if (op == QLatin1String("REMOVE") && r.from == QLatin1String("*"))
            {
                res.slotPaths.clear();
                res.operations << QStringLiteral("Remove:*");
                continue;
            }

            if (op == QLatin1String("ADD") && isLayerGroup(r.from))
            {
                const QStringList base = builtinLayerSlots();
                for (const QString& s : base)
                {
                    if (res.slotPaths.contains(s))
                        continue;
                    // Remove * cleared the active selection; re-attach whatever
                    // texture the material already assigned to this slot so a
                    // template application never drops existing texture paths.
                    res.slotPaths.insert(s, slotMap.value(s));
                }
                res.operations << QStringLiteral("Add:") + r.from;
                continue;
            }

            QString slot = textureSlotOf(r.from);
            if (slot.isEmpty() && !r.from.contains(QLatin1Char('/')) &&
                !r.from.contains(QLatin1Char('*')))
                slot = r.from;   // bare slot-name rule

            if (slot.isEmpty())
                continue;         // property-group rule, not a texture slot

            if (op == QLatin1String("ADD"))
            {
                if (!res.slotPaths.contains(slot))
                    res.slotPaths.insert(slot, QString());
                log(r, slot);
            }
            else if (op == QLatin1String("REMOVE"))
            {
                res.slotPaths.remove(slot);
                log(r, slot);
            }
            else if (op == QLatin1String("MAKECONST"))
            {
                res.slotPaths.remove(slot);   // becomes a constant, no texture needed
                log(r, slot);
            }
            else if (op == QLatin1String("MOVE"))
            {
                const QString toSlot = textureSlotOf(r.to);
                if (res.slotPaths.contains(slot) && !toSlot.isEmpty() &&
                    res.slotPaths.value(toSlot).isEmpty() && toSlot != slot)
                {
                    const QString path = res.slotPaths.value(slot);
                    res.slotPaths.remove(slot);
                    res.slotPaths.insert(toSlot, path);
                    log(r, slot + QLatin1Char('>') + toSlot);
                }
                else
                {
                    log(r, slot);
                }
            }
            else
            {
                log(r, slot);
            }
        }
    }
    return res;
}

MaterialRuleTemplate::ApplyResult
MaterialRuleTemplate::applyToSlots(const QStringList& slotNames) const
{
    QMap<QString, QString> map;
    for (const QString& s : slotNames)
        map.insert(s, QString());
    ApplyMapResult r = applyToSlotMap(map);
    ApplyResult out;
    out.slotNames = r.slotPaths.keys();
    out.operations = r.operations;
    return out;
}

QStringList MaterialRuleTemplate::requiredSlots() const
{
    return applyToSlots(QStringList()).slotNames;
}

QStringList MaterialRuleTemplate::builtinNames()
{
    return { QStringLiteral("1LayerStandard"),
             QStringLiteral("2LayerStandard"),
             QStringLiteral("3LayerStandard"),
             QStringLiteral("4LayerStandard"),
             QStringLiteral("Terrain"),
             QStringLiteral("Skin"),
             QStringLiteral("Hair"),
             QStringLiteral("Eye"),
             QStringLiteral("Water"),
             QStringLiteral("Vegetation") };
}

MaterialRuleTemplate MaterialRuleTemplate::builtinTemplate(const QString& name)
{
    MaterialRuleTemplate tpl;
    tpl.category = QStringLiteral("ShaderModels");
    tpl.name = name;
    tpl.displayName = name;
    tpl.rootMaterial = name;
    tpl.version = QStringLiteral("1");

    RuleClass rc;
    rc.className = QStringLiteral("BSMaterial::LayeredMaterialID");
    Rule wipe; wipe.from = QStringLiteral("*"); wipe.op = QStringLiteral("Remove");
    Rule add;  add.from = QStringLiteral("Layer1"); add.op = QStringLiteral("Add");
    rc.rules = { wipe, add };
    tpl.ruleClasses = { rc };
    return tpl;
}
