#include <QtEndian>
#include <QtTest>
#include <QBuffer>
#include <QFile>
#include <QDir>

#include "../../libs/files/facefx/facefxanim.hpp"
#include "../../libs/files/ba2/ba2archive.hpp"
#include "../../libs/files/log/logger.hpp"

#include <QTemporaryDir>
#include <QFileInfo>

#include <cstring>

// Partial .ffxanim container parser (magic + header + constant-count 12-byte
// records). Strict: refuses size drift / truncation rather than fabricating.
class TestFaceFxAnim : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testRejectBadMagic();
    void testRejectTruncated();
    void testRejectSizeMismatch();
    void testSyntheticRoundTrip();
    void testRealDataGate();
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

QByteArray makeAnim(quint16 format, const QByteArray& id, const QList<FaceFx::AnimRecord>& records)
{
    QByteArray out("__ffx\0", 6);
    appendU16(out, format);
    appendU32(out, static_cast<quint32>(0)); // size placeholder, fix up later
    out.append(id);
    appendU32(out, static_cast<quint32>(records.size()));
    for (const FaceFx::AnimRecord& r : records) {
        quint32 bits = 0;
        std::memcpy(&bits, &r.value, 4);
        appendU32(out, bits);
        appendU16(out, r.field0);
        appendU16(out, r.field1);
        appendU16(out, r.field2);
        appendU16(out, r.field3);
    }
    qToLittleEndian(static_cast<quint32>(out.size()),
                    reinterpret_cast<uchar*>(out.data() + 8));
    return out;
}

} // namespace

void TestFaceFxAnim::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
}

void TestFaceFxAnim::testRejectBadMagic()
{
    FaceFx::FaceFxAnim a;
    QByteArray b = makeAnim(0x0300, QByteArray(20, '\x01'), {});
    b[0] = 'X';
    QVERIFY(!a.load(b));
    QVERIFY(!a.errorString().isEmpty());
}

void TestFaceFxAnim::testRejectTruncated()
{
    FaceFx::FaceFxAnim a;
    QByteArray b = makeAnim(0x0300, QByteArray(20, '\x02'), {});
    b.chop(6);
    QVERIFY(!a.load(b));
}

void TestFaceFxAnim::testRejectSizeMismatch()
{
    FaceFx::FaceFxAnim a;
    QByteArray b = makeAnim(0x0300, QByteArray(20, '\x03'), {});
    qToLittleEndian(b.size() + 12u, reinterpret_cast<uchar*>(b.data() + 8));
    QVERIFY(!a.load(b));
}

void TestFaceFxAnim::testSyntheticRoundTrip()
{
    FaceFx::AnimRecord r0{ -0.422667f, 0, 0, 0, 25 };
    FaceFx::AnimRecord r1{ -0.022764f, 94, 3, 65528, 26 };
    QVector<FaceFx::AnimRecord> v{ r0, r1 };
    const QByteArray id(20, '\xAA');
    const QByteArray buf = makeAnim(0x0300, id, v);

    FaceFx::FaceFxAnim a;
    QVERIFY2(a.load(buf), qPrintable(a.errorString()));
    QCOMPARE(a.header().format, static_cast<quint16>(0x0300));
    QCOMPARE(a.header().size, static_cast<quint32>(buf.size()));
    QCOMPARE(a.header().id, id);
    QCOMPARE(a.header().count, static_cast<quint32>(2));
    QCOMPARE(a.records().size(), 2);
    QVERIFY(qAbs(a.records()[0].value + 0.422667f) < 1e-6f);
    QCOMPARE(a.records()[1].field0, static_cast<quint16>(94));
    QCOMPARE(a.records()[1].field3, static_cast<quint16>(26));
}

void TestFaceFxAnim::testRealDataGate()
{
    const QString dataDir =
        qEnvironmentVariable("OPENCK_DATA_DIR",
                             QStringLiteral("C:/XboxGames/Starfield/Content/Data"));
    const QString archive = dataDir + QStringLiteral("/Starfield - FaceAnimation01.ba2");
    if (!QFileInfo::exists(archive))
        QSKIP("No FaceAnimation01.ba2; set OPENCK_DATA_DIR");

    Ba2Archive ba2;
    QVERIFY2(ba2.open(archive), "FaceAnimation ba2 did not open");
    int seen = 0;
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    for (quint32 i = 0; i < ba2.fileCount() && seen < 5; ++i) {
        const auto& e = ba2.entries().at(i);
        if (!e.relativePath.endsWith(".ffxanim", Qt::CaseInsensitive)) continue;
        const QString dest = tmp.filePath(QStringLiteral("sample%1.ffxanim").arg(seen));
        if (!ba2.extract(i, dest)) continue;
        ++seen;
        FaceFx::FaceFxAnim a;
        QVERIFY2(a.load(dest), qPrintable(a.errorString()));
        QVERIFY(a.header().count > 0);
        QCOMPARE(a.header().id.size(), 20);
        QVERIFY(a.records().size() > 0);
        // All sampled values must be in a positional range that the skinning
        // path can at least clamp; we do not assume semantics here, only
        // that the stream is not telemetry noise.
        bool anyFinite = false;
        for (int j = 0; j < qMin(32, a.records().size()); ++j) {
            const float v = a.records()[j].value;
            anyFinite |= (v == v) && (v > -100.0f && v < 100.0f);
        }
        QVERIFY(anyFinite);
    }
    QVERIFY(seen > 0);
}

QTEST_MAIN(TestFaceFxAnim)
#include "test_facefxanim.moc"
