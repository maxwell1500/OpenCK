#include "objectwindowfilter.hpp"

#include <QJsonArray>
#include <QRegularExpression>

FilterRule::Type FilterRule::typeFromString(const QString& text)
{
    const QString lower = text.trimmed().toLower();
    if (lower == QStringLiteral("equalto") || lower == QStringLiteral("==")
        || lower == QStringLiteral("=") || lower == QStringLiteral("equals")
        || lower == QStringLiteral("exact") || lower == QStringLiteral("exactvalue"))
        return Type::Equals;
    if (lower == QStringLiteral("notequals") || lower == QStringLiteral("notequalto")
        || lower == QStringLiteral("!="))
        return Type::NotEquals;
    if (lower == QStringLiteral("contains"))
        return Type::Contains;
    if (lower == QStringLiteral("startswith"))
        return Type::StartsWith;
    if (lower == QStringLiteral("endswith"))
        return Type::EndsWith;
    if (lower == QStringLiteral("range") || lower == QStringLiteral("inrange")
        || lower == QStringLiteral("between"))
        return Type::Range;
    if (lower == QStringLiteral("regex") || lower == QStringLiteral("regexmatch"))
        return Type::RegexMatch;
    if (lower == QStringLiteral("wildcard") || lower == QStringLiteral("wildcardmatch"))
        return Type::Wildcard;
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
    case Type::Wildcard:    return QStringLiteral("Wildcard");
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
        if (exactValue.contains(QLatin1Char('*')) || exactValue.contains(QLatin1Char('?')))
        {
            const QRegularExpression re(QRegularExpression::wildcardToRegularExpression(exactValue),
                                         QRegularExpression::CaseInsensitiveOption);
            matched = re.isValid() && re.match(value).hasMatch();
        }
        else
        {
            matched = value.compare(exactValue, Qt::CaseInsensitive) == 0;
        }
        break;
    case Type::Wildcard:
    {
        const QRegularExpression re(QRegularExpression::wildcardToRegularExpression(exactValue),
                                     QRegularExpression::CaseInsensitiveOption);
        matched = re.isValid() && re.match(value).hasMatch();
        break;
    }
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
    case Type::Wildcard:
        if (exactValue.contains(QLatin1Char('*')) || exactValue.contains(QLatin1Char('?')))
        {
            const QRegularExpression re(QRegularExpression::wildcardToRegularExpression(exactValue),
                                         QRegularExpression::CaseInsensitiveOption);
            matched = re.isValid() && re.match(QString::number(value)).hasMatch();
        }
        else
        {
            matched = (value == exactValue.toDouble());
        }
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
    rule.isConcatenatedOr = obj.value(QStringLiteral("IsConcatenatedOr")).toBool(false);
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
    obj.insert(QStringLiteral("IsConcatenatedOr"), isConcatenatedOr);
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
    for (const FilterRule& rule : rules)
    {
        if (rule.enabled)
            ++enabled;
    }
    if (enabled == 0)
        return true;

    auto rawMatches = [](const FilterRule& rule, const QJsonValue& item) -> bool {
        FilterRule pos = rule;
        pos.isNegative = false;
        if (item.isDouble())
            return pos.matchesNumber(item.toDouble());
        return pos.matches(item.toString());
    };

    auto evalRule = [&](const FilterRule& rule) -> bool {
        const QJsonValue val = record.value(rule.parameter);
        if (val.isArray())
        {
            bool anyItemMatched = false;
            for (const QJsonValue& item : val.toArray())
            {
                if (rawMatches(rule, item))
                {
                    anyItemMatched = true;
                    break;
                }
            }
            return rule.isNegative ? !anyItemMatched : anyItemMatched;
        }
        if (val.isDouble())
            return rule.matchesNumber(val.toDouble());
        return rule.matches(val.toString());
    };

    if (isConcatenatedOr)
    {
        for (const FilterRule& rule : rules)
        {
            if (!rule.enabled) continue;
            if (evalRule(rule))
                return true;
        }
        return false;
    }

    bool currentAndGroup = true;
    bool anyGroupMatched = false;
    bool hasOr = false;

    for (const FilterRule& rule : rules)
    {
        if (!rule.enabled)
            continue;
        const bool hit = evalRule(rule);
        currentAndGroup = currentAndGroup && hit;

        if (rule.isConcatenatedOr)
        {
            hasOr = true;
            if (currentAndGroup)
                anyGroupMatched = true;
            currentAndGroup = true;
        }
    }

    if (hasOr)
        return anyGroupMatched || currentAndGroup;

    return currentAndGroup;
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
        if (value.isArray())
        {
            FilterRule pos = rule;
            pos.isNegative = false;
            bool anyItemMatched = false;
            for (const QJsonValue& item : value.toArray())
            {
                bool itemHit = item.isDouble() ? pos.matchesNumber(item.toDouble())
                                               : pos.matches(item.toString());
                if (itemHit)
                {
                    anyItemMatched = true;
                    break;
                }
            }
            hit = rule.isNegative ? !anyItemMatched : anyItemMatched;
        }
        else if (value.isDouble())
        {
            hit = rule.matchesNumber(value.toDouble());
        }
        else
        {
            hit = rule.matches(value.toString());
        }
        if (hit)
            return true;
    }
    return false;
}
