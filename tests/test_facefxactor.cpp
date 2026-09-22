#include <QtTest>
#include <QtEndian>
#include <QBuffer>
#include <QDataStream>
#include <QFile>
#include <QDir>

#include "../../libs/files/facefx/facefxactor.hpp"
#include "../../libs/files/log/logger.hpp"

using FaceFx::FaceFxActor;

// Partial .facefx container parser (type table + node names). Strict:
// rejects bad magic, truncated headers, and missing tables rather than
// guessing. Real-actor coverage is gated on OPENCK_TEST_FACEFX_DIR so the
// suite runs without shipping middleware samples.
class TestFaceFxActor : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testRejectBadMagic();
    void testRejectTruncated();
    void testSyntheticRoundTrip();
    void testRejectMissingType();
    void testRejectMissingNameTable();
    void testRealActors();
};

namespace {

void appendU32(QByteArray& b, quint32 v)
{
    char buf[4];
    qToLittleEndian(v, reinterpret_cast<uchar*>(buf));
    b.append(buf, 4);
}

void appendU16(QByteArray& b, quint16 v)
{
    char buf[2];
    qToLittleEndian(v, reinterpret_cast<uchar*>(buf));
    b.append(buf, 2);
}

void appendStr0(QByteArray& b, const QByteArray& s)
{
    appendU32(b, static_cast<quint32>(s.size() + 1));
    b.append(s);
    b.append('\0');
}

QByteArray makeActor(quint32 version, const QList<QPair<quint32, QByteArray>>& types,
                     const QList<QPair<quint32, QByteArray>>& names)
{
    QByteArray out("FACE", 4);
    appendU32(out, version);
    appendU32(out, 13);
    appendStr0(out, "ZeniMax Media");
    appendStr0(out, "EFG");
    appendU32(out, 1000);
    appendU16(out, 2);
    appendU32(out, static_cast<quint32>(types.size()));
    appendU32(out, static_cast<quint32>(types.size()));
    appendU32(out, 7);
    appendU32(out, 1);
    for (const auto& t : types) {
        appendU32(out, t.first);
        appendU32(out, static_cast<quint32>(t.second.size()));
        out.append(t.second);
        // 28-byte type payload (common layout).
        out.append(QByteArray(12, '\xFF'));
        appendU16(out, 0);
        appendU32(out, 0);
        appendU32(out, 0);
        appendU16(out, 0);
        appendU32(out, 0);
        appendU32(out, 0);
    }
    for (const auto& n : names) {
        appendU32(out, n.first);
        appendU32(out, static_cast<quint32>(n.second.size()));
        out.append(n.second);
        appendU32(out, 1);
        appendU32(out, 2);
        appendU32(out, 3);
    }
    appendU32(out, 0xFFFFFFFFu);
    return out;
}

} // namespace

void TestFaceFxActor::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(
        QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_facefxactor_log.txt"));
}

void TestFaceFxActor::testRejectBadMagic()
{
    FaceFxActor a;
    QVERIFY(!a.load(QByteArray()));
    QVERIFY(!a.load(QByteArrayLiteral("NOPE")));
    QVERIFY(!a.load(QByteArrayLiteral("FACE"))); // too short
    QVERIFY(!a.errorString().isEmpty());
}

void TestFaceFxActor::testRejectTruncated()
{
    QByteArray b("FACE", 4);
    appendU32(b, 2171);
    appendU32(b, 13);
    FaceFxActor a;
    QVERIFY(!a.load(b));
    QVERIFY(!a.errorString().isEmpty());
}

void TestFaceFxActor::testSyntheticRoundTrip()
{
    const QByteArray bytes = makeActor(
        2171,
        { { 0, "FxBone" }, { 1, "FxActor" }, { 2, "FxName" } },
        { { 5, "browLowererL" }, { 3, "Eye Yaw" }, { 0, "TestActor" } });

    FaceFxActor a;
    QVERIFY2(a.load(bytes), qPrintable(a.errorString()));
    QCOMPARE(a.header().version, 2171u);
    QCOMPARE(a.header().typeCount, 3u);
    QCOMPARE(a.header().publisher, QStringLiteral("ZeniMax Media"));
    QCOMPARE(a.types().size(), 3);
    QCOMPARE(a.types().at(0).name, QStringLiteral("FxBone"));
    QCOMPARE(a.types().at(1).name, QStringLiteral("FxActor"));
    QCOMPARE(a.types().at(2).name, QStringLiteral("FxName"));
    QCOMPARE(a.names().size(), 3);
    QCOMPARE(a.names().at(0).name, QStringLiteral("browLowererL"));
    QCOMPARE(a.names().at(1).name, QStringLiteral("Eye Yaw"));
    QCOMPARE(a.names().at(2).name, QStringLiteral("TestActor"));
    QCOMPARE(a.names().at(0).payload[0], 1u);
    QCOMPARE(a.names().at(0).payload[1], 2u);
    QCOMPARE(a.names().at(0).payload[2], 3u);
}

void TestFaceFxActor::testRejectMissingType()
{
    // Header claims 3 types but only 1 is present.
    QByteArray out("FACE", 4);
    appendU32(out, 2171);
    appendU32(out, 13);
    appendStr0(out, "ZeniMax Media");
    appendStr0(out, "EFG");
    appendU32(out, 1000);
    appendU16(out, 2);
    appendU32(out, 3); // typeCount = 3
    appendU32(out, 3);
    appendU32(out, 7);
    appendU32(out, 1);
    appendU32(out, 0);
    appendU32(out, 6);
    out.append("FxBone");
    out.append(QByteArray(28, '\0'));

    FaceFxActor a;
    QVERIFY(!a.load(out));
    QVERIFY(a.errorString().contains(QStringLiteral("type entry")));
}

void TestFaceFxActor::testRejectMissingNameTable()
{
    const QByteArray bytes = makeActor(2171, { { 0, "FxBone" } }, {});
    // Strip the trailing terminator so no name run can be found.
    QByteArray truncated = bytes;
    // makeActor with empty names still appends terminator; remove last 4 bytes.
    if (truncated.size() >= 4)
        truncated.chop(4);

    FaceFxActor a;
    QVERIFY(!a.load(truncated));
    QVERIFY(!a.errorString().isEmpty());
}

void TestFaceFxActor::testRealActors()
{
    const QString dir = qEnvironmentVariable("OPENCK_TEST_FACEFX_DIR");
    if (dir.isEmpty())
        QSKIP("OPENCK_TEST_FACEFX_DIR not set");

    const QStringList files = QDir(dir).entryList({ QStringLiteral("*.facefx") },
                                                   QDir::Files);
    QVERIFY2(!files.isEmpty(), "no .facefx files in OPENCK_TEST_FACEFX_DIR");

    for (const QString& f : files) {
        FaceFxActor a;
        const QString path = dir + QLatin1Char('/') + f;
        QVERIFY2(a.load(path), qPrintable(f + ": " + a.errorString()));
        QVERIFY2(a.header().typeCount > 0, qPrintable(f));
        QCOMPARE(static_cast<quint32>(a.types().size()), a.header().typeCount);
        QVERIFY2(!a.names().isEmpty(), qPrintable(f + ": no names"));
        // Every type name is Fx-prefixed.
        for (const auto& t : a.types())
            QVERIFY2(t.name.startsWith(QLatin1String("Fx")),
                     qPrintable(f + ": bad type " + t.name));
        // Names are non-empty printable.
        for (const auto& n : a.names())
            QVERIFY2(!n.name.isEmpty(), qPrintable(f + ": empty name"));
        qDebug() << f << "types=" << a.types().size() << "names=" << a.names().size()
                 << "ver=" << a.header().version;
    }
}

QTEST_MAIN(TestFaceFxActor)
#include "test_facefxactor.moc"
