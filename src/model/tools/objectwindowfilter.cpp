#include "objectwindowfilter.hpp"

#include <QJsonArray>
#include <QRegularExpression>

FilterRule::Type FilterRule::typeFromString(const QString& text)
{
    const QString lower = text.trimmed().toLower();
    if (lower == QStringLiteral("equalto") || lower == QStringLiteral("==")
        || lower == QStringLiteral("="))
        return Type::Equals;
    if (lower == QStringLiteral("notequalto") || lower == QStringLiteral("!="))
        return Type::NotEquals;
    if (lower == QStringLiteral("startswith"))
        return Type::StartsWith;
    if (lower == QStringLiteral("endswith"))
        return Type::EndsWith;
    if (lower == QStringLiteral("range") || lower == QStringLiteral("inrange")
        || lower == QStringLiteral("between"))
        return Type::Range;
    if (lower == QStringLiteral("regex") || lower == QStringLiteral("regexmatch"))
        return Type::RegexMatch;
    return Type::Contains;  // "contains" and anything unknown
}

QString FilterRule::typeToString(Type type)
{
    switch (type)
    {
    case Type::Equals:      return QStringLiteral("EqualTo");
    case Type::NotEquals:   return QStringLiteral("NotEqualTo");
    case Type::StartsWith:  return QStringLiteral("StartsWith");
    case Type::EndsWith:    return QStringLiteral("EndsWith");
    case Type::Range:       return QStringLiteral("Range");
    case Type::RegexMatch:  return QStringLiteral("RegexMatch");
    case Type::Contains:    return QStringLiteral("Contains");
    }
    return QStringLiteral("Contains");
}

bool FilterRule::matches(const QString& value) const
{
    bool matched = false;
    switch (type)
    {
    case Type::Equals:
        matched = value.compare(exactValue, Qt::CaseInsensitive) == 0;
        break;
    case Type::NotEquals:
        matched = value.compare(exactValue, Qt::CaseInsensitive) != 0;
        break;
    case Type::Contains:
        matched = value.contains(exactValue, Qt::CaseInsensitive);
        break;
    case Type::StartsWith:
        matched = value.startsWith(exactValue, Qt::CaseInsensitive);
        break;
    case Type::EndsWith:
        matched = value.endsWith(exactValue, Qt::CaseInsensitive);
        break;
    case Type::Range:
    {
        bool ok = false;
        const double number = value.toDouble(&ok);
        matched = ok && number >= minValue && number <= maxValue;
        break;
    }
    case Type::RegexMatch:
    {
        const QRegularExpression re(exactValue,
            QRegularExpression::CaseInsensitiveOption);
        matched = re.isValid() && re.match(value).hasMatch();
        break;
    }
    }
    return isNegative ? !matched : matched;
}

bool FilterRule::matchesNumber(double value) const
{
    bool matched = false;
    switch (type)
    {
    case Type::Equals:
    case Type::NotEquals:
        matched = (value == exactValue.toDouble());
        if (type == Type::NotEquals)
            matched = !matched;
        break;
    case Type::Range:
        matched = value >= minValue && value <= maxValue;
        break;
    default:
        // Text comparison against a number: format as string.
        matched = matches(QString::number(value));
        break;
    }
    return isNegative ? !matched : matched;
}

FilterRule FilterRule::fromJson(const QJsonObject& obj)
{
    FilterRule rule;
    rule.parameter = obj.value(QStringLiteral("ParameterName")).toString();
    if (rule.parameter.isEmpty())
        rule.parameter = obj.value(QStringLiteral("parameter")).toString();
    rule.type = typeFromString(
        obj.value(QStringLiteral("FilterType")).toString());
    rule.exactValue = obj.value(QStringLiteral("ExactValue")).toString();
    if (rule.exactValue.isEmpty())
        rule.exactValue = obj.value(QStringLiteral("value")).toString();
    rule.minValue = obj.value(QStringLiteral("MinValue")).toDouble(0.0);
    rule.maxValue = obj.value(QStringLiteral("MaxValue")).toDouble(0.0);
    rule.isNegative = obj.value(QStringLiteral("IsNegative")).toBool(false);
    rule.enabled = !obj.value(QStringLiteral("IsEnabled")).isBool()
        || obj.value(QStringLiteral("IsEnabled")).toBool();
    return rule;
}

QJsonObject FilterRule::toJson() const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("ParameterName"), parameter);
    obj.insert(QStringLiteral("FilterType"), typeToString(type));
    obj.insert(QStringLiteral("ExactValue"), exactValue);
    obj.insert(QStringLiteral("MinValue"), minValue);
    obj.insert(QStringLiteral("MaxValue"), maxValue);
    obj.insert(QStringLiteral("IsNegative"), isNegative);
    return obj;
}

ObjectWindowFilter ObjectWindowFilter::fromJson(const QJsonValue& value)
{
    ObjectWindowFilter filter;
    if (value.isObject())
    {
        const QJsonObject obj = value.toObject();
        // Full file form: { "IsConcatenatedOr": bool, "Rules": [...] }.
        if (obj.contains(QStringLiteral("IsConcatenatedOr")))
            filter.isConcatenatedOr =
                obj.value(QStringLiteral("IsConcatenatedOr")).toBool();
        QJsonValue rules = obj.value(QStringLiteral("Rules"));
        if (rules.isUndefined())
            rules = obj.value(QStringLiteral("rules"));
        if (rules.isArray())
        {
            for (const QJsonValue& item : rules.toArray())
            {
                if (item.isObject())
                    filter.addRule(FilterRule::fromJson(item.toObject()));
            }
        }
        else if (obj.contains(QStringLiteral("ParameterName")))
        {
            // A bare rule object.
            filter.addRule(FilterRule::fromJson(obj));
        }
    }
    else if (value.isArray())
    {
        for (const QJsonValue& item : value.toArray())
        {
            if (item.isObject())
                filter.addRule(FilterRule::fromJson(item.toObject()));
        }
    }
    return filter;
}

QJsonObject ObjectWindowFilter::toJson() const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("IsConcatenatedOr"), isConcatenatedOr);
    QJsonArray arr;
    for (const FilterRule& rule : rules)
        arr.append(rule.toJson());
    obj.insert(QStringLiteral("Rules"), arr);
    return obj;
}

void ObjectWindowFilter::addRule(const FilterRule& rule)
{
    rules.append(rule);
}

void ObjectWindowFilter::clear()
{
    rules.clear();
}

bool ObjectWindowFilter::matches(const QJsonObject& record) const
{
    int enabled = 0;
    bool anyMatch = false;
    for (const FilterRule& rule : rules)
    {
        if (!rule.enabled)
            continue;
        ++enabled;
        const bool hit = matches(rule.parameter, record.value(rule.parameter));
        if (isConcatenatedOr)
        {
            if (hit)
                anyMatch = true;
        }
        else if (!hit)
        {
            return false;
        }
    }
    if (enabled == 0)
        return true;
    if (isConcatenatedOr)
        return anyMatch;
    return true;
}

bool ObjectWindowFilter::matches(const QString& parameter,
                                 const QJsonValue& value) const
{
    for (const FilterRule& rule : rules)
    {
        if (!rule.enabled)
            continue;
        if (!rule.parameter.isEmpty()
            && rule.parameter.compare(parameter, Qt::CaseInsensitive) != 0)
            continue;
        bool hit = false;
        if (value.isDouble())
            hit = rule.matchesNumber(value.toDouble());
        else
            hit = rule.matches(value.toString());
        if (hit)
            return true;
    }
    return false;
}
