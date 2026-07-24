#include <QtTest>
#include <QRegularExpression>

// Standalone implementation of matchesCriteria for testing
// (mirrors SearchAlgorithm::matchesCriteria from searchalgorithm.cpp)

enum class MatchMode { Contains, StartsWith, EndsWith, Exact, Regex };

bool matchesCriteria(const QString& value, const QString& searchText, MatchMode mode, bool caseSensitive)
{
    if (searchText.isEmpty()) return true;
    if (value.isEmpty()) return false;
    
    QString val = caseSensitive ? value : value.toLower();
    QString search = caseSensitive ? searchText : searchText.toLower();
    
    switch (mode)
    {
    case MatchMode::Contains:
        return val.contains(search);
    case MatchMode::StartsWith:
        return val.startsWith(search);
    case MatchMode::EndsWith:
        return val.endsWith(search);
    case MatchMode::Exact:
        return val == search;
    case MatchMode::Regex:
    {
        QRegularExpression regex(search, caseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
        return regex.match(val).hasMatch();
    }
    }
    return false;
}

class TestSearchAlgorithm : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testMatchesCriteria_Contains();
    void testMatchesCriteria_StartsWith();
    void testMatchesCriteria_EndsWith();
    void testMatchesCriteria_Exact();
    void testMatchesCriteria_Regex();
    void testMatchesCriteria_CaseSensitive();
    void testMatchesCriteria_EmptySearch();
};

void TestSearchAlgorithm::initTestCase()
{
}

void TestSearchAlgorithm::cleanupTestCase()
{
}

void TestSearchAlgorithm::testMatchesCriteria_Contains()
{
    QVERIFY(matchesCriteria("TestNPC", "test", MatchMode::Contains, false));
    QVERIFY(matchesCriteria("TestNPC", "NPC", MatchMode::Contains, false));
    QVERIFY(!matchesCriteria("TestNPC", "xyz", MatchMode::Contains, false));
}

void TestSearchAlgorithm::testMatchesCriteria_StartsWith()
{
    QVERIFY(matchesCriteria("TestNPC", "test", MatchMode::StartsWith, false));
    QVERIFY(!matchesCriteria("TestNPC", "NPC", MatchMode::StartsWith, false));
}

void TestSearchAlgorithm::testMatchesCriteria_EndsWith()
{
    QVERIFY(matchesCriteria("TestNPC", "npc", MatchMode::EndsWith, false));
    QVERIFY(!matchesCriteria("TestNPC", "test", MatchMode::EndsWith, false));
}

void TestSearchAlgorithm::testMatchesCriteria_Exact()
{
    QVERIFY(matchesCriteria("TestNPC", "testnpc", MatchMode::Exact, false));
    QVERIFY(!matchesCriteria("TestNPC01", "testnpc", MatchMode::Exact, false));
}

void TestSearchAlgorithm::testMatchesCriteria_Regex()
{
    QVERIFY(matchesCriteria("TestNPC01", "TestNPC\\d+", MatchMode::Regex, false));
    QVERIFY(!matchesCriteria("TestNPC", "TestNPC\\d+", MatchMode::Regex, false));
}

void TestSearchAlgorithm::testMatchesCriteria_CaseSensitive()
{
    QVERIFY(matchesCriteria("TestNPC", "test", MatchMode::Contains, false));
    QVERIFY(!matchesCriteria("TestNPC", "test", MatchMode::Contains, true));
    QVERIFY(matchesCriteria("TestNPC", "Test", MatchMode::Contains, true));
}

void TestSearchAlgorithm::testMatchesCriteria_EmptySearch()
{
    // Empty search text matches everything (implementation behavior)
    QVERIFY(matchesCriteria("TestNPC", "", MatchMode::Contains, false));
}

QTEST_MAIN(TestSearchAlgorithm)
#include "test_searchalgorithm.moc"
