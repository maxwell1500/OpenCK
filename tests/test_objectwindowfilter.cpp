#include <QTest>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include "../../src/model/tools/objectwindowfilter.hpp"

class TestObjectWindowFilter : public QObject
{
    Q_OBJECT

private slots:
    void testRuleTypeParsing();
    void testRuleMatches();
    void testRuleNumberMatches();
    void testRuleJsonRoundTrip();
    void testFilterAnd();
    void testFilterOr();
    void testFilterFullFileForm();
    void testFilterArrayForm();
    void testFilterEmpty();
    void testFilterDisabledRules();
    void testFilterNegation();
    void testFilterRegex();
};

void TestObjectWindowFilter::testRuleTypeParsing()
{
    QCOMPARE(FilterRule::typeFromString(QStringLiteral("EqualTo")), FilterRule::Type::Equals);
    QCOMPARE(FilterRule::typeFromString(QStringLiteral("NotEqualTo")), FilterRule::Type::NotEquals);
    QCOMPARE(FilterRule::typeFromString(QStringLiteral("Contains")), FilterRule::Type::Contains);
    QCOMPARE(FilterRule::typeFromString(QStringLiteral("StartsWith")), FilterRule::Type::StartsWith);
    QCOMPARE(FilterRule::typeFromString(QStringLiteral("EndsWith")), FilterRule::Type::EndsWith);
    QCOMPARE(FilterRule::typeFromString(QStringLiteral("Range")), FilterRule::Type::Range);
    QCOMPARE(FilterRule::typeFromString(QStringLiteral("RegexMatch")), FilterRule::Type::RegexMatch);
    QCOMPARE(FilterRule::typeFromString(QStringLiteral("garbage")), FilterRule::Type::Contains);
    QCOMPARE(FilterRule::typeToString(FilterRule::Type::StartsWith), QStringLiteral("StartsWith"));
}

void TestObjectWindowFilter::testRuleMatches()
{
    FilterRule contains;
    contains.parameter = QStringLiteral("Name");
    contains.type = FilterRule::Type::Contains;
    contains.exactValue = QStringLiteral("sword");
    QVERIFY(contains.matches(QStringLiteral("Daedric Sword")));
    QVERIFY(!contains.matches(QStringLiteral("Axe")));

    FilterRule equals;
    equals.type = FilterRule::Type::Equals;
    equals.exactValue = QStringLiteral("Weapon");
    QVERIFY(equals.matches(QStringLiteral("weapon")));
    QVERIFY(!equals.matches(QStringLiteral("Armor")));

    FilterRule starts;
    starts.type = FilterRule::Type::StartsWith;
    starts.exactValue = QStringLiteral("Ile");
    QVERIFY(starts.matches(QStringLiteral("IleXHarbor")));
    QVERIFY(!starts.matches(QStringLiteral("Harbor")));
}

void TestObjectWindowFilter::testRuleNumberMatches()
{
    FilterRule range;
    range.type = FilterRule::Type::Range;
    range.minValue = 10.0;
    range.maxValue = 20.0;
    QVERIFY(range.matchesNumber(15.0));
    QVERIFY(!range.matchesNumber(5.0));

    FilterRule equals;
    equals.type = FilterRule::Type::Equals;
    equals.exactValue = QStringLiteral("4");
    QVERIFY(equals.matchesNumber(4.0));
    QVERIFY(!equals.matchesNumber(5.0));
}

void TestObjectWindowFilter::testRuleJsonRoundTrip()
{
    QJsonObject json;
    json.insert(QStringLiteral("ParameterName"), QStringLiteral("Type"));
    json.insert(QStringLiteral("FilterType"), QStringLiteral("EqualTo"));
    json.insert(QStringLiteral("ExactValue"), QStringLiteral("Weapon"));
    json.insert(QStringLiteral("IsNegative"), true);

    const FilterRule rule = FilterRule::fromJson(json);
    QCOMPARE(rule.parameter, QStringLiteral("Type"));
    QCOMPARE(rule.type, FilterRule::Type::Equals);
    QCOMPARE(rule.exactValue, QStringLiteral("Weapon"));
    QVERIFY(rule.isNegative);

    const QJsonObject round = rule.toJson();
    QCOMPARE(round.value(QStringLiteral("ParameterName")).toString(), QStringLiteral("Type"));
    QCOMPARE(round.value(QStringLiteral("FilterType")).toString(), QStringLiteral("EqualTo"));
    QCOMPARE(round.value(QStringLiteral("ExactValue")).toString(), QStringLiteral("Weapon"));
    QVERIFY(round.value(QStringLiteral("IsNegative")).toBool());
}

void TestObjectWindowFilter::testFilterAnd()
{
    ObjectWindowFilter filter;
    FilterRule type;
    type.parameter = QStringLiteral("Type");
    type.type = FilterRule::Type::Equals;
    type.exactValue = QStringLiteral("Weapon");
    FilterRule name;
    name.parameter = QStringLiteral("Name");
    name.type = FilterRule::Type::Contains;
    name.exactValue = QStringLiteral("sword");
    filter.addRule(type);
    filter.addRule(name);

    QJsonObject record;
    record.insert(QStringLiteral("Type"), QStringLiteral("Weapon"));
    record.insert(QStringLiteral("Name"), QStringLiteral("Iron Sword"));
    QVERIFY(filter.matches(record));

    record.insert(QStringLiteral("Name"), QStringLiteral("Iron Axe"));
    QVERIFY(!filter.matches(record));

    record.insert(QStringLiteral("Type"), QStringLiteral("Armor"));
    record.insert(QStringLiteral("Name"), QStringLiteral("Iron Sword"));
    QVERIFY(!filter.matches(record));
}

void TestObjectWindowFilter::testFilterOr()
{
    ObjectWindowFilter filter;
    filter.isConcatenatedOr = true;
    FilterRule a;
    a.parameter = QStringLiteral("Type");
    a.type = FilterRule::Type::Equals;
    a.exactValue = QStringLiteral("Weapon");
    FilterRule b;
    b.parameter = QStringLiteral("Type");
    b.type = FilterRule::Type::Equals;
    b.exactValue = QStringLiteral("Armor");
    filter.addRule(a);
    filter.addRule(b);

    QJsonObject record;
    record.insert(QStringLiteral("Type"), QStringLiteral("Armor"));
    QVERIFY(filter.matches(record));
    record.insert(QStringLiteral("Type"), QStringLiteral("Clothing"));
    QVERIFY(!filter.matches(record));
}

void TestObjectWindowFilter::testFilterFullFileForm()
{
    const QByteArray text = R"({
        "IsConcatenatedOr": false,
        "Rules": [
            {"ParameterName": "Type", "FilterType": "EqualTo", "ExactValue": "Weapon"},
            {"ParameterName": "Name", "FilterType": "StartsWith", "ExactValue": "Iron"}
        ]
    })";
    const ObjectWindowFilter filter =
        ObjectWindowFilter::fromJson(QJsonDocument::fromJson(text).object());
    QVERIFY(!filter.isConcatenatedOr);
    QCOMPARE(filter.count(), 2);
    QVERIFY(filter.matches(QJsonObject{
        { QStringLiteral("Type"), QStringLiteral("Weapon") },
        { QStringLiteral("Name"), QStringLiteral("Iron Sword") } }));
    QVERIFY(!filter.matches(QJsonObject{
        { QStringLiteral("Type"), QStringLiteral("Weapon") },
        { QStringLiteral("Name"), QStringLiteral("Steel Sword") } }));
}

void TestObjectWindowFilter::testFilterArrayForm()
{
    const QByteArray text = R"([
        {"ParameterName": "Name", "FilterType": "Contains", "ExactValue": "scroll"}
    ])";
    const ObjectWindowFilter filter =
        ObjectWindowFilter::fromJson(QJsonDocument::fromJson(text).array());
    QCOMPARE(filter.count(), 1);
    QVERIFY(filter.matches(QJsonObject{
        { QStringLiteral("Name"), QStringLiteral("Fire Scroll") } }));
    QVERIFY(!filter.matches(QJsonObject{
        { QStringLiteral("Name"), QStringLiteral("Potion") } }));
}

void TestObjectWindowFilter::testFilterEmpty()
{
    ObjectWindowFilter filter;
    // An empty filter matches everything (no-op).
    QVERIFY(filter.matches(QJsonObject{}));
    QVERIFY(filter.matches(QJsonObject{
        { QStringLiteral("Type"), QStringLiteral("Whatever") } }));
}

void TestObjectWindowFilter::testFilterDisabledRules()
{
    ObjectWindowFilter filter;
    FilterRule rule;
    rule.parameter = QStringLiteral("Type");
    rule.type = FilterRule::Type::Equals;
    rule.exactValue = QStringLiteral("Weapon");
    rule.enabled = false;
    filter.addRule(rule);
    QVERIFY(filter.matches(QJsonObject{
        { QStringLiteral("Type"), QStringLiteral("Armor") } }));
}

void TestObjectWindowFilter::testFilterNegation()
{
    ObjectWindowFilter filter;
    FilterRule rule;
    rule.parameter = QStringLiteral("Type");
    rule.type = FilterRule::Type::Equals;
    rule.exactValue = QStringLiteral("Weapon");
    rule.isNegative = true;
    filter.addRule(rule);
    QVERIFY(filter.matches(QJsonObject{
        { QStringLiteral("Type"), QStringLiteral("Armor") } }));
    QVERIFY(!filter.matches(QJsonObject{
        { QStringLiteral("Type"), QStringLiteral("Weapon") } }));
}

void TestObjectWindowFilter::testFilterRegex()
{
    ObjectWindowFilter filter;
    FilterRule rule;
    rule.parameter = QStringLiteral("Name");
    rule.type = FilterRule::Type::RegexMatch;
    rule.exactValue = QStringLiteral("^Iron.*Sword$");
    filter.addRule(rule);
    QVERIFY(filter.matches(QJsonObject{
        { QStringLiteral("Name"), QStringLiteral("Iron Sword") } }));
    QVERIFY(!filter.matches(QJsonObject{
        { QStringLiteral("Name"), QStringLiteral("Iron Axe") } }));
}

QTEST_MAIN(TestObjectWindowFilter)
#include "test_objectwindowfilter.moc"
