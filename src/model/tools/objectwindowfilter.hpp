#ifndef OBJECTWINDOWFILTER_H
#define OBJECTWINDOWFILTER_H

#include <QString>
#include <QJsonObject>
#include <QJsonValue>
#include <QVector>

// ObjectWindowFilter implements data-driven Object Window keyword filtering
// using the same JSON schema as the real Creation Kit's DataViews
// ObjectWindow _common/*.filter files. A filter is a list of rules combined
// with AND (or OR when IsConcatenatedOr is set); each rule picks a record
// parameter (ParameterName), a comparison (FilterType) and a value
// (ExactValue, or MinValue/MaxValue for range rules), and may be negated
// (IsNegative). Rules are matched against a record's JSON view so the same
// filter works across every record type.
struct FilterRule
{
    enum class Type
    {
        Equals,          // EqualTo
        NotEquals,       // NotEqualTo
        Contains,        // Contains
        StartsWith,      // StartsWith
        EndsWith,        // EndsWith
        Range,           // MinValue..MaxValue
        RegexMatch       // RegexMatch
    };

    QString parameter;        // ParameterName, e.g. "EditorID", "Name", "Type"
    Type type = Type::Contains;
    QString exactValue;       // ExactValue
    double minValue = 0.0;    // range lower bound
    double maxValue = 0.0;    // range upper bound
    bool isNegative = false;  // IsNegative (invert the match)
    bool enabled = true;

    static Type typeFromString(const QString& text);
    static QString typeToString(Type type);

    // Matches a single string value (case-insensitive for text rules).
    bool matches(const QString& value) const;
    // Matches a numeric value (range/equality rules only).
    bool matchesNumber(double value) const;

    // Parses one rule from its JSON form. Unknown filter types are treated
    // as Contains.
    static FilterRule fromJson(const QJsonObject& obj);
    QJsonObject toJson() const;
};

class ObjectWindowFilter
{
public:
    // True => rules combine with OR; false (default) => AND.
    bool isConcatenatedOr = false;
    QString name;

    QVector<FilterRule> rules;

    // Parses a filter from JSON. Accepts either a bare rule object, an array
    // of rules, or the full file form { "IsConcatenatedOr": bool, "Rules":
    // [...] } / { "rules": [...] }.
    static ObjectWindowFilter fromJson(const QJsonValue& value);

    void addRule(const FilterRule& rule);
    void clear();
    int count() const { return rules.size(); }

    // Evaluates the filter against a record's JSON view (keys are matched by
    // ParameterName, values may be strings or numbers). Disabled rules are
    // skipped. Returns true when no enabled rules exist (filter is a no-op).
    bool matches(const QJsonObject& record) const;
    bool matches(const QString& parameter, const QJsonValue& value) const;
};

#endif // OBJECTWINDOWFILTER_H
