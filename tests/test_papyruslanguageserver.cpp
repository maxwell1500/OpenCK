#include <QTest>
#include <QJsonDocument>

#include "../../src/model/tools/papyruslanguageserver.hpp"

class TestPapyrusLanguageServer : public QObject
{
    Q_OBJECT

private slots:
    void testFrameMessage();
    void testParseSingle();
    void testParseMultiple();
    void testParseSplit();
    void testRequestAndNotification();
};

void TestPapyrusLanguageServer::testFrameMessage()
{
    QJsonObject body;
    body.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    body.insert(QStringLiteral("id"), 1);
    body.insert(QStringLiteral("method"), QStringLiteral("initialize"));

    const QByteArray framed = PapyrusLanguageServer::frameMessage(body);
    QVERIFY(framed.startsWith("Content-Length: "));
    QVERIFY(framed.contains("\r\n\r\n"));

    // The payload after the header must match the body exactly.
    const int headerEnd = framed.indexOf("\r\n\r\n") + 4;
    const QJsonDocument doc = QJsonDocument::fromJson(framed.mid(headerEnd));
    QVERIFY(doc.isObject());
    QCOMPARE(doc.object().value(QStringLiteral("method")).toString(),
             QStringLiteral("initialize"));
}

void TestPapyrusLanguageServer::testParseSingle()
{
    QJsonObject body;
    body.insert(QStringLiteral("id"), 3);
    body.insert(QStringLiteral("result"), QJsonObject());

    QByteArray buffer = PapyrusLanguageServer::frameMessage(body);
    const QList<QJsonObject> messages = PapyrusLanguageServer::parseMessages(buffer);
    QCOMPARE(messages.size(), 1);
    QCOMPARE(messages[0].value(QStringLiteral("id")).toInt(), 3);
    // The buffer is fully consumed.
    QVERIFY(buffer.isEmpty());
}

void TestPapyrusLanguageServer::testParseMultiple()
{
    QJsonObject a;
    a.insert(QStringLiteral("id"), 1);
    QJsonObject b;
    b.insert(QStringLiteral("id"), 2);

    QByteArray buffer = PapyrusLanguageServer::frameMessage(a)
        + PapyrusLanguageServer::frameMessage(b);
    const QList<QJsonObject> messages = PapyrusLanguageServer::parseMessages(buffer);
    QCOMPARE(messages.size(), 2);
    QCOMPARE(messages[0].value(QStringLiteral("id")).toInt(), 1);
    QCOMPARE(messages[1].value(QStringLiteral("id")).toInt(), 2);
}

void TestPapyrusLanguageServer::testParseSplit()
{
    QJsonObject body;
    body.insert(QStringLiteral("method"), QStringLiteral("textDocument/hover"));

    const QByteArray framed = PapyrusLanguageServer::frameMessage(body);

    // Feed the header only; nothing should parse yet.
    QByteArray buffer = framed.left(framed.size() / 2);
    QVERIFY(PapyrusLanguageServer::parseMessages(buffer).isEmpty());

    // Feed the rest; the message must now be found.
    buffer.append(framed.mid(framed.size() / 2));
    const QList<QJsonObject> messages = PapyrusLanguageServer::parseMessages(buffer);
    QCOMPARE(messages.size(), 1);
    QCOMPARE(messages[0].value(QStringLiteral("method")).toString(),
             QStringLiteral("textDocument/hover"));
}

void TestPapyrusLanguageServer::testRequestAndNotification()
{
    const QJsonObject req = PapyrusLanguageServer::request(
        QStringLiteral("textDocument/completion"), QJsonObject(), 7);
    QCOMPARE(req.value(QStringLiteral("jsonrpc")).toString(), QStringLiteral("2.0"));
    QCOMPARE(req.value(QStringLiteral("id")).toInt(), 7);
    QCOMPARE(req.value(QStringLiteral("method")).toString(),
             QStringLiteral("textDocument/completion"));

    const QJsonObject note = PapyrusLanguageServer::notification(
        QStringLiteral("textDocument/didOpen"), QJsonObject());
    QVERIFY(!note.contains(QStringLiteral("id")));
    QCOMPARE(note.value(QStringLiteral("method")).toString(),
             QStringLiteral("textDocument/didOpen"));
}

QTEST_MAIN(TestPapyrusLanguageServer)
#include "test_papyruslanguageserver.moc"
