#include "textureconversionrules.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>

#include "libs/files/log/logger.hpp"

QString TextureConversionRule::formatToString(Format f)
{
    switch (f)
    {
    case Format::BC7: return QStringLiteral("BC7");
    case Format::BC5: return QStringLiteral("BC5");
    case Format::BC4: return QStringLiteral("BC4");
    case Format::R8: return QStringLiteral("R8");
    case Format::R8G8B8A8: return QStringLiteral("R8G8B8A8");
    case Format::R8G8B8A8_UNORM_SRGB: return QStringLiteral("R8G8B8A8_UNORM_SRGB");
    case Format::Unknown: break;
    }
    return QStringLiteral("UNKNOWN");
}

TextureConversionRule::Format TextureConversionRule::stringToFormat(
    const QString& text)
{
    const QString upper = text.trimmed().toUpper();
    if (upper == QStringLiteral("BC7") || upper == QStringLiteral("BC7_UNORM")
        || upper == QStringLiteral("DXGI_FORMAT_BC7_UNORM"))
        return Format::BC7;
    if (upper == QStringLiteral("BC5") || upper == QStringLiteral("BC5_UNORM")
        || upper == QStringLiteral("DXGI_FORMAT_BC5_UNORM"))
        return Format::BC5;
    if (upper == QStringLiteral("BC4") || upper == QStringLiteral("BC4_UNORM")
        || upper == QStringLiteral("DXGI_FORMAT_BC4_UNORM"))
        return Format::BC4;
    if (upper == QStringLiteral("R8") || upper == QStringLiteral("R8_UNORM"))
        return Format::R8;
    if (upper == QStringLiteral("R8G8B8A8") || upper == QStringLiteral("R8G8B8A8_UNORM"))
        return Format::R8G8B8A8;
    if (upper == QStringLiteral("R8G8B8A8_UNORM_SRGB"))
        return Format::R8G8B8A8_UNORM_SRGB;
    return Format::Unknown;
}

TextureConversionRule TextureConversionRule::fromJson(const QJsonObject& obj)
{
    TextureConversionRule rule;
    rule.pathPattern = obj.value(QStringLiteral("path")).toString(
        obj.value(QStringLiteral("pattern")).toString());

    const QString fmt = obj.value(QStringLiteral("format")).toString();
    rule.format = stringToFormat(fmt);

    rule.generateMipmaps = obj.value(QStringLiteral("mipmaps")).toBool(true);
    if (obj.contains(QStringLiteral("generateMipmaps")))
        rule.generateMipmaps = obj.value(QStringLiteral("generateMipmaps")).toBool(true);

    rule.srgb = obj.value(QStringLiteral("srgb")).toBool(
        obj.value(QStringLiteral("gamma")).toBool(false));
    rule.distanceField = obj.value(QStringLiteral("distanceField")).toBool(false);
    rule.physicallyBasedMipmaps =
        obj.value(QStringLiteral("physicallyBasedMipmaps")).toBool(false);
    rule.compressUberFallback =
        obj.value(QStringLiteral("compressUberFallback")).toBool(false);
    rule.maxTextureSize =
        obj.value(QStringLiteral("maxTextureSize")).toInt(0);
    return rule;
}

bool TextureConversionRules::parse(const QString& json,
                                   QVector<TextureConversionRule>& out)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError)
    {
        LOG_WARNING(QString("TextureConversionRules: JSON error at %1: %2")
                        .arg(error.offset).arg(error.errorString()));
        return false;
    }

    if (doc.isArray())
    {
        const QJsonArray arr = doc.array();
        for (const QJsonValue& v : arr)
        {
            if (v.isObject())
                out.append(TextureConversionRule::fromJson(v.toObject()));
        }
        return !out.isEmpty();
    }

    if (doc.isObject())
    {
        const QJsonObject root = doc.object();
        QJsonArray rules;
        if (root.contains(QStringLiteral("rules")))
            rules = root.value(QStringLiteral("rules")).toArray();
        else if (root.contains(QStringLiteral("textures")))
            rules = root.value(QStringLiteral("textures")).toArray();
        else if (root.contains(QStringLiteral("settings")))
        {
            // xtexconv settings: per-extension settings objects.
            const QJsonObject settings = root.value(QStringLiteral("settings")).toObject();
            for (auto it = settings.begin(); it != settings.end(); ++it)
            {
                if (!it.value().isObject())
                    continue;
                QJsonObject ruleObj = it.value().toObject();
                ruleObj.insert(QStringLiteral("path"), it.key());
                out.append(TextureConversionRule::fromJson(ruleObj));
            }
            return !out.isEmpty();
        }

        for (const QJsonValue& v : rules)
        {
            if (v.isObject())
                out.append(TextureConversionRule::fromJson(v.toObject()));
        }
        return !out.isEmpty();
    }

    return false;
}

bool TextureConversionRules::loadFile(const QString& path,
                                      QVector<TextureConversionRule>& out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_WARNING(QString("TextureConversionRules: cannot open %1").arg(path));
        return false;
    }
    const QString json = QString::fromUtf8(file.readAll());
    return parse(json, out);
}

const TextureConversionRule* TextureConversionRules::findRule(
    const QVector<TextureConversionRule>& rules, const QString& texturePath)
{
    const QString normalized = texturePath.toLower();
    for (const TextureConversionRule& rule : rules)
    {
        if (rule.pathPattern.isEmpty())
            continue;
        // Build a simple glob from the pattern (lowercased).
        QString pattern = rule.pathPattern.toLower();
        QString regex;
        regex.reserve(pattern.size() + 16);
        for (const QChar c : pattern)
        {
            if (c == QLatin1Char('*'))
                regex += QStringLiteral(".*");
            else if (c == QLatin1Char('?'))
                regex += QLatin1Char('.');
            else
                regex += QRegularExpression::escape(QString(c));
        }
        const QRegularExpression re(regex, QRegularExpression::CaseInsensitiveOption);
        if (re.isValid() && re.match(normalized).hasMatch())
            return &rule;
    }
    return nullptr;
}

QVector<TextureConversionRule> TextureConversionRules::builtin()
{
    QVector<TextureConversionRule> rules;

    TextureConversionRule r;
    r.pathPattern = QStringLiteral("textures/actors/character/*");
    r.format = TextureConversionRule::Format::BC7;
    r.generateMipmaps = true;
    r.srgb = true;
    rules.append(r);

    r = TextureConversionRule();
    r.pathPattern = QStringLiteral("*_n.dds");
    r.format = TextureConversionRule::Format::BC5;
    r.generateMipmaps = true;
    r.srgb = false;
    rules.append(r);

    r = TextureConversionRule();
    r.pathPattern = QStringLiteral("textures/terrain/*");
    r.format = TextureConversionRule::Format::BC7;
    r.generateMipmaps = true;
    r.srgb = false;
    rules.append(r);

    r = TextureConversionRule();
    r.pathPattern = QStringLiteral("textures/water/*");
    r.format = TextureConversionRule::Format::BC7;
    r.generateMipmaps = true;
    r.srgb = false;
    r.physicallyBasedMipmaps = true;
    rules.append(r);

    return rules;
}
