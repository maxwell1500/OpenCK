#include <QTest>
#include <QJsonDocument>

#include "../../src/model/tools/papyrusremotedebugger.hpp"

class TestPapyrusRemoteDebugger : public QObject
{
    Q_OBJECT

private slots:
    void testBuildCommandSimple();
    void testBuildCommandWithArgs();
    void testParseResponse();
    void testDefaultPort();
};

void TestPapyrusRemoteDebugger::testBuildCommandSimple()
{
    const QString cmd = PapyrusRemoteDebugger::buildCommand(QStringLiteral("step"));
    const QJsonDocument doc = QJsonDocument::fromJson(cmd.toUtf8());
    QVERIFY(doc.isObject());
    QCOMPARE(doc.object().value(QStringLiteral("cmd")).toString(),
             QStringLiteral("step"));
}

void TestPapyrusRemoteDebugger::testBuildCommandWithArgs()
{
    QVariantMap args;
    args.insert(QStringLiteral("script"), QStringLiteral("MyScript"));
    args.insert(QStringLiteral("line"), 12);
    const QString cmd = PapyrusRemoteDebugger::buildCommand(
        QStringLiteral("setBreakpoint"), args);

    const QJsonDocument doc = QJsonDocument::fromJson(cmd.toUtf8());
    QVERIFY(doc.isObject());
    const QJsonObject obj = doc.object();
    QCOMPARE(obj.value(QStringLiteral("cmd")).toString(), QStringLiteral("setBreakpoint"));
    QCOMPARE(obj.value(QStringLiteral("script")).toString(), QStringLiteral("MyScript"));
    QCOMPARE(obj.value(QStringLiteral("line")).toInt(), 12);
}

void TestPapyrusRemoteDebugger::testParseResponse()
{
    const QString line = QStringLiteral("{\"type\":\"locals\",\"vars\":{\"x\":\"1\"}}");
    const QVariantMap map = PapyrusRemoteDebugger::parseResponse(line);
    QCOMPARE(map.value(QStringLiteral("type")).toString(), QStringLiteral("locals"));
    QVERIFY(map.contains(QStringLiteral("vars")));

    // Non-JSON input parses to an empty map (no crash).
    QVERIFY(PapyrusRemoteDebugger::parseResponse(QStringLiteral("not json")).isEmpty());
}

void TestPapyrusRemoteDebugger::testDefaultPort()
{
    QCOMPARE(PapyrusRemoteDebugger::kDefaultPort, static_cast<quint16>(20548));
}

QTEST_MAIN(TestPapyrusRemoteDebugger)
#include "test_papyrusremotedebugger.moc"
