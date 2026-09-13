#include <QTest>

#include "obscriptlexer.hpp"

using namespace ObScript;

class TestObScript : public QObject
{
    Q_OBJECT

private slots:
    void testKeywordsAndIdentifiers();
    void testNumbers();
    void testStrings();
    void testOperators();
    void testNewlinesAndComments();
    void testFullScript();
};

void TestObScript::testKeywordsAndIdentifiers()
{
    Lexer lx("int health = 100");
    const QVector<Token> toks = lx.tokenizeAll();
    QVERIFY(!lx.hasError());

    QVector<TokenKind> got;
    for (const Token& t : toks)
    {
        got.append(t.kind);
    }
    const QVector<TokenKind> exp = {TokenKind::Keyword, TokenKind::Identifier,
                                    TokenKind::Operator, TokenKind::Integer,
                                    TokenKind::End};
    QCOMPARE(got, exp);
    QCOMPARE(toks.at(0).text, QString("int"));
    QCOMPARE(toks.at(1).text, QString("health"));
    QCOMPARE(toks.at(2).text, QString("="));
    QCOMPARE(toks.at(3).text, QString("100"));
}

void TestObScript::testNumbers()
{
    Lexer lx("0x1F 42 3.14 1.5e3 .5 2.0f");
    const QVector<Token> toks = lx.tokenizeAll();
    QVERIFY(!lx.hasError());

    QCOMPARE(toks.at(0).kind, TokenKind::Integer);
    QCOMPARE(toks.at(0).text, QString("0x1F"));
    QCOMPARE(toks.at(1).kind, TokenKind::Integer);
    QCOMPARE(toks.at(1).text, QString("42"));
    QCOMPARE(toks.at(2).kind, TokenKind::Floating);
    QCOMPARE(toks.at(2).text, QString("3.14"));
    QCOMPARE(toks.at(3).kind, TokenKind::Floating);
    QCOMPARE(toks.at(3).text, QString("1.5e3"));
    QCOMPARE(toks.at(4).kind, TokenKind::Floating);
    QCOMPARE(toks.at(4).text, QString(".5"));
    QCOMPARE(toks.at(5).kind, TokenKind::Floating);
    QCOMPARE(toks.at(5).text, QString("2.0f"));
}

void TestObScript::testStrings()
{
    Lexer lx("msg \"Line\\nBreak\"");
    const QVector<Token> toks = lx.tokenizeAll();
    QVERIFY(!lx.hasError());

    QCOMPARE(toks.at(0).kind, TokenKind::Identifier);
    QCOMPARE(toks.at(0).text, QString("msg"));
    QCOMPARE(toks.at(1).kind, TokenKind::String);
    QCOMPARE(toks.at(1).text, QString("Line\nBreak"));
}

void TestObScript::testOperators()
{
    Lexer lx("a == b && c <= d ? e : f");
    const QVector<Token> toks = lx.tokenizeAll();
    QVERIFY(!lx.hasError());

    QString ops;
    for (const Token& t : toks)
    {
        if (t.kind == TokenKind::Operator)
        {
            ops += t.text;
        }
    }
    QCOMPARE(ops, QString("==&&<=?:"));
}

void TestObScript::testNewlinesAndComments()
{
    Lexer lx("x = 1 ; trailing\ny = 2\n");
    const QVector<Token> toks = lx.tokenizeAll();
    QVERIFY(!lx.hasError());

    QVector<TokenKind> got;
    for (const Token& t : toks)
    {
        got.append(t.kind);
    }
    const QVector<TokenKind> exp = {TokenKind::Identifier, TokenKind::Operator,
                                    TokenKind::Integer, TokenKind::NewLine,
                                    TokenKind::Identifier, TokenKind::Operator,
                                    TokenKind::Integer, TokenKind::NewLine,
                                    TokenKind::End};
    QCOMPARE(got, exp);
}

void TestObScript::testFullScript()
{
    const QString src = QString::fromUtf8(
        "Begin Script\n"
        "\tint i = 0\n"
        "\twhile i < 10\n"
        "\t\ti += 1\n"
        "\tEndWhile\n"
        "EndScript\n");
    Lexer lx(src);
    const QVector<Token> toks = lx.tokenizeAll();
    QVERIFY(!lx.hasError());

    const auto has = [&toks](TokenKind k, const QString& text) {
        for (const Token& t : toks)
        {
            if (t.kind == k && t.text == text)
            {
                return true;
            }
        }
        return false;
    };
    QVERIFY(has(TokenKind::Keyword, "begin"));
    QVERIFY(has(TokenKind::Keyword, "script"));
    QVERIFY(has(TokenKind::Keyword, "int"));
    QVERIFY(has(TokenKind::Keyword, "while"));
    QVERIFY(has(TokenKind::Keyword, "endwhile"));
    QVERIFY(has(TokenKind::Keyword, "endscript"));
    QVERIFY(has(TokenKind::Operator, "+="));
    QVERIFY(has(TokenKind::Identifier, "i"));
    QVERIFY(has(TokenKind::Integer, "10"));
    QVERIFY(has(TokenKind::Integer, "1"));
}

QTEST_MAIN(TestObScript)
#include "test_obscript.moc"
