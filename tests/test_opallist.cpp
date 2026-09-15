#include <QtTest>
#include <QFile>
#include <QFileInfo>
#include <QDir>

#include <cstring>

#include "../../src/model/tools/opallist.hpp"
#include "logger.hpp"

// Validates the real binary OPAL (.opl) codec (REMAINING.md §3.8). The pure
// slots pin the layout; testRealShippedLists runs against the ten lists that
// ship under Content/OPAL and asserts a byte-exact round-trip of every one.
class TestOpalList : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testSyntheticRoundTrip();
    void testSyntheticNoTransform();
    void testMalformedRejected();
    void testLoadMissingFile();
    void testRealShippedLists();
};

namespace {

QByteArray leU32(quint32 v)
{
    QByteArray b;
    b.append(static_cast<char>(v & 0xFF));
    b.append(static_cast<char>((v >> 8) & 0xFF));
    b.append(static_cast<char>((v >> 16) & 0xFF));
    b.append(static_cast<char>((v >> 24) & 0xFF));
    return b;
}

QByteArray leF32(float f)
{
    quint32 bits = 0;
    std::memcpy(&bits, &f, sizeof(bits));
    return leU32(bits);
}

// Builds one entry in the on-disk layout.
QByteArray entry(const QByteArray& name, const QByteArray& payload, quint32 formId)
{
    QByteArray b = leU32(static_cast<quint32>(name.size()));
    b.append(name);
    b.append('\0');
    b.append(leU32(static_cast<quint32>(payload.size())));
    b.append(payload);
    b.append(leU32(formId));
    b.append(leU32(0));   // trailer high 32 bits
    return b;
}

} // namespace

void TestOpalList::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_opallist_log.txt"));
}

void TestOpalList::testSyntheticRoundTrip()
{
    QByteArray payload;
    payload.append(leF32(1.5f));
    payload.append(leF32(-2.25f));
    payload.append(leF32(3.0f));
    payload.append(leF32(0.0f));
    payload.append(leF32(0.5f));
    payload.append(leF32(-0.5f));

    QByteArray data;
    data.append(leU32(3));           // version
    data.append(leU32(2));           // count
    data.append(entry("Bar_Bowl02", payload, 0x00278522u));
    data.append(entry("NoTransform", QByteArray(), 0x00278523u));

    OpalList list;
    QVERIFY(OpalList::parse(data, list));
    QCOMPARE(list.version, quint32(3));
    QCOMPARE(list.rowCount(), 2);

    QCOMPARE(list.placements[0].name, QStringLiteral("Bar_Bowl02"));
    QVERIFY(list.placements[0].hasTransform());
    QCOMPARE(list.placements[0].formId(), quint32(0x00278522));
    const QVector<float> t = list.placements[0].transform();
    QCOMPARE(t.size(), 6);
    QCOMPARE(t[0], 1.5f);
    QCOMPARE(t[1], -2.25f);
    QCOMPARE(t[4], 0.5f);

    QCOMPARE(list.placements[1].name, QStringLiteral("NoTransform"));
    QVERIFY(!list.placements[1].hasTransform());
    QCOMPARE(list.placements[1].payload.size(), 0);
    QCOMPARE(list.placements[1].formId(), quint32(0x00278523));

    // Serialize must reproduce the exact input bytes.
    QCOMPARE(list.serialize(), data);

    // And parsing the serialized form must match again.
    OpalList again;
    QVERIFY(OpalList::parse(list.serialize(), again));
    QCOMPARE(again.serialize(), data);
}

void TestOpalList::testSyntheticNoTransform()
{
    QByteArray data;
    data.append(leU32(3));
    data.append(leU32(1));
    data.append(entry("OnlyName", QByteArray(), 0x00000001u));

    OpalList list;
    QVERIFY(OpalList::parse(data, list));
    QCOMPARE(list.rowCount(), 1);
    QCOMPARE(list.placements[0].transform().size(), 0);
    QCOMPARE(list.serialize(), data);
}

void TestOpalList::testMalformedRejected()
{
    OpalList list;

    // Empty buffer.
    QVERIFY(!OpalList::parse(QByteArray(), list));

    // Declared count with a truncated entry.
    QByteArray truncated;
    truncated.append(leU32(3));
    truncated.append(leU32(1));
    truncated.append(leU32(5));   // nameLen 5 but no bytes follow
    QVERIFY(!OpalList::parse(truncated, list));

    // Trailing garbage past the declared count.
    QByteArray trailing;
    trailing.append(leU32(3));
    trailing.append(leU32(0));
    trailing.append("junk", 4);
    QVERIFY(!OpalList::parse(trailing, list));

    // A failed parse must leave the output empty.
    QVERIFY(list.rowCount() == 0);
}

void TestOpalList::testLoadMissingFile()
{
    OpalList list;
    QVERIFY(!OpalList::loadFile(QStringLiteral("Z:/missing.opl"), list));
}

void TestOpalList::testRealShippedLists()
{
    const QString contentDir =
        qEnvironmentVariable("OPENCK_DATA_DIR",
                             QStringLiteral("C:/XboxGames/Starfield/Content/Data"));
    QDir dataDir(contentDir);
    dataDir.cdUp();   // Content/
    QDir opalDir(dataDir.filePath(QStringLiteral("OPAL")));
    if (!opalDir.exists())
        QSKIP("No Content/OPAL directory; set OPENCK_DATA_DIR");

    const QStringList files =
        opalDir.entryList({QStringLiteral("*.opl")}, QDir::Files, QDir::Name);
    if (files.isEmpty())
        QSKIP("No .opl files found");

    int totalPlacements = 0;
    int withTransform = 0;
    for (const QString& name : files)
    {
        const QString path = opalDir.filePath(name);
        OpalList list;
        QVERIFY2(OpalList::loadFile(path, list), qPrintable(name));
        QVERIFY(!list.placements.isEmpty());

        // Byte-exact round-trip of the shipped file.
        QFile original(path);
        QVERIFY(original.open(QIODevice::ReadOnly));
        const QByteArray bytes = original.readAll();
        original.close();
        QCOMPARE(list.serialize(), bytes);

        for (const OpalPlacement& p : list.placements)
        {
            if (!p.hasTransform())
                continue;
            ++withTransform;
            const QVector<float> t = p.transform();
            QCOMPARE(t.size(), 6);
        }
        totalPlacements += list.rowCount();
    }

    qDebug() << "OPAL files:" << files.size()
             << "placements:" << totalPlacements
             << "with transform:" << withTransform;
    QVERIFY(files.size() >= 10);
    QVERIFY(totalPlacements > 3000);
    QVERIFY(withTransform > 3000);
}

QTEST_MAIN(TestOpalList)
#include "test_opallist.moc"
