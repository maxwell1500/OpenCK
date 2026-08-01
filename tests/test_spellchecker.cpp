#include <QTest>
#include <QTemporaryFile>
#include <QFile>

#include "../../src/model/tools/spellchecker.hpp"

class TestSpellChecker : public QObject
{
    Q_OBJECT

private slots:
    void testAddAndIsKnown();
    void testCaseInsensitive();
    void testUnknownWords();
    void testLoadDictionary();
    void testSuggestions();
};

void TestSpellChecker::testAddAndIsKnown()
{
    SpellChecker checker;
    checker.addWord(QStringLiteral("hello"));
    checker.addWord(QStringLiteral("world"));
    QCOMPARE(checker.wordCount(), 2);
    QVERIFY(checker.isKnown(QStringLiteral("hello")));
    QVERIFY(!checker.isKnown(QStringLiteral("hllo")));
    checker.addWord(QStringLiteral("   ")); // empty -> ignored
    QCOMPARE(checker.wordCount(), 2);
}

void TestSpellChecker::testCaseInsensitive()
{
    SpellChecker checker;
    checker.addWord(QStringLiteral("DragonBorn"));
    QVERIFY(checker.isKnown(QStringLiteral("dragonborn")));
    QVERIFY(checker.isKnown(QStringLiteral("DRAGONBORN")));
    QVERIFY(checker.isKnown(QStringLiteral("DragonBorn")));
}

void TestSpellChecker::testUnknownWords()
{
    SpellChecker checker;
    checker.addWord(QStringLiteral("the"));
    checker.addWord(QStringLiteral("quick"));
    checker.addWord(QStringLiteral("fox"));

    const QStringList unknown = checker.unknownWords(
        QStringLiteral("The quik fox 123 jumps"));
    QCOMPARE(unknown, QStringList({ QStringLiteral("quik"), QStringLiteral("jumps") }));
}

void TestSpellChecker::testLoadDictionary()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write("alpha\n");
    file.write("; comment line\n");
    file.write("# another comment\n");
    file.write("beta\n");
    file.write("\n");
    file.write("gamma\n");
    const QString path = file.fileName();
    file.close();

    SpellChecker checker;
    const int added = checker.loadDictionary(path);
    QCOMPARE(added, 3);
    QCOMPARE(checker.wordCount(), 3);
    QVERIFY(checker.isKnown(QStringLiteral("alpha")));
    QVERIFY(checker.isKnown(QStringLiteral("gamma")));
    QVERIFY(!checker.isKnown(QStringLiteral("comment")));

    // Missing file adds nothing.
    SpellChecker empty;
    QCOMPARE(empty.loadDictionary(QStringLiteral("Z:/nope.txt")), 0);
}

void TestSpellChecker::testSuggestions()
{
    SpellChecker checker;
    checker.addWord(QStringLiteral("hello"));
    checker.addWord(QStringLiteral("help"));
    checker.addWord(QStringLiteral("shell"));
    checker.addWord(QStringLiteral("world"));

    const QStringList suggestions = checker.suggestions(QStringLiteral("hllo"));
    QVERIFY(!suggestions.isEmpty());
    // "hello" (1 edit) ranks before "help"/"shell" (2 edits).
    QCOMPARE(suggestions.first(), QStringLiteral("hello"));
    QVERIFY(suggestions.contains(QStringLiteral("help"))
            || suggestions.contains(QStringLiteral("shell")));

    // Exact matches are not suggested.
    QVERIFY(!suggestions.contains(QStringLiteral("hllo")));
}

QTEST_MAIN(TestSpellChecker)
#include "test_spellchecker.moc"
