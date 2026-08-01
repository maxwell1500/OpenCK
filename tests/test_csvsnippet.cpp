#include <QTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>

#include "../../libs/files/data/csvsnippet.hpp"
#include "../../libs/files/log/logger.hpp"

class TestCsvSnippet : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testSubstituteBasic();
    void testSubstituteCount();
    void testSubstituteNested();
    void testRenderComments();
    void testRenderImport();
    void testMissingAccessorEmpty();
};

void TestCsvSnippet::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_csvsnippet_log.txt"));
}

static QJsonObject sampleRecord()
{
    QJsonObject obj;
    obj["editorId"] = "NPC001";
    obj["formId"] = "0x00001234";
    obj["level"] = 10;
    obj["health"] = 120.5;

    QJsonObject pos;
    pos["X"] = 1.0;
    pos["Y"] = 2.0;
    obj["position"] = pos;

    QJsonArray list;
    list.append(1);
    list.append(2);
    list.append(3);
    obj["effects"] = list;
    return obj;
}

void TestCsvSnippet::testSubstituteBasic()
{
    const QVariantMap fields = CsvSnippet::flattenRecord(sampleRecord());
    QCOMPARE(CsvSnippet::substituteLine("id=<editorId> lvl=<level>", fields),
        QStringLiteral("id=NPC001 lvl=10"));
    QCOMPARE(CsvSnippet::substituteLine("<Type:health>", fields),
        QStringLiteral("120.5"));
}

void TestCsvSnippet::testSubstituteCount()
{
    const QVariantMap fields = CsvSnippet::flattenRecord(sampleRecord());
    QCOMPARE(CsvSnippet::substituteLine("n=<effects.Count>", fields),
        QStringLiteral("n=3"));
}

void TestCsvSnippet::testSubstituteNested()
{
    const QVariantMap fields = CsvSnippet::flattenRecord(sampleRecord());
    QCOMPARE(CsvSnippet::substituteLine("<position.X>,<position.Y>", fields),
        QStringLiteral("1,2"));
}

void TestCsvSnippet::testRenderComments()
{
    QTemporaryDir dir;
    const QString path = dir.filePath("snippet.txt");
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    f.write("# a comment\n\n<NPC_:editorId>,<level>\n");
    f.close();

    const QVector<CsvSnippet::Row> rows = CsvSnippet::render(path, sampleRecord());
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows[0].text, QStringLiteral("NPC001,10"));
}

void TestCsvSnippet::testRenderImport()
{
    QTemporaryDir dir;
    const QString inner = dir.filePath("inner.txt");
    QFile fi(inner);
    QVERIFY(fi.open(QIODevice::WriteOnly | QIODevice::Text));
    fi.write("inner=<editorId>\n");
    fi.close();

    const QString outer = dir.filePath("outer.txt");
    QFile fo(outer);
    QVERIFY(fo.open(QIODevice::WriteOnly | QIODevice::Text));
    fo.write("first\n.Import=inner.txt\nlast\n");
    fo.close();

    const QVector<CsvSnippet::Row> rows = CsvSnippet::render(outer, sampleRecord());
    QCOMPARE(rows.size(), 3);
    QCOMPARE(rows[0].text, QStringLiteral("first"));
    QCOMPARE(rows[1].text, QStringLiteral("inner=NPC001"));
    QCOMPARE(rows[2].text, QStringLiteral("last"));
}

void TestCsvSnippet::testMissingAccessorEmpty()
{
    const QVariantMap fields = CsvSnippet::flattenRecord(sampleRecord());
    QCOMPARE(CsvSnippet::substituteLine("<missing>", fields), QStringLiteral(""));
    QCOMPARE(CsvSnippet::substituteLine("x=<missing>y", fields), QStringLiteral("x=y"));
}

QTEST_MAIN(TestCsvSnippet)
#include "test_csvsnippet.moc"
