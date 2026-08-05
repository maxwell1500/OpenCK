#include <QTest>
#include <QTemporaryFile>
#include <QFile>
#include <cstring>

#include "../../libs/files/esm/aniorecord.hpp"
#include "../../libs/files/esm/artvrecord.hpp"
#include "../../libs/files/esm/clfmrecord.hpp"
#include "../../libs/files/esm/debrrecord.hpp"
#include "../../libs/files/esm/ecznrecord.hpp"
#include "../../libs/files/esm/hazdrecord.hpp"
#include "../../libs/files/esm/ipctrecord.hpp"
#include "../../libs/files/esm/ipdsrecord.hpp"
#include "../../libs/files/esm/mustrecord.hpp"
#include "../../libs/files/esm/relarecord.hpp"
#include "../../libs/files/esm/revbrecord.hpp"
#include "../../libs/files/esm/shourecord.hpp"
#include "../../libs/files/esm/hdptrecord.hpp"
#include "../../libs/files/esm/termrecord.hpp"
#include "../../libs/files/esm/mattrecord.hpp"
#include "../../libs/files/esm/movtrecord.hpp"
#include "../../libs/files/esm/muscrecord.hpp"
#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/log/logger.hpp"

class TestMissingRecords : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testRelaRoundTrip();
    void testDebrRoundTrip();
    void testHazdRoundTrip();
    void testShouRoundTrip();
    void testClfmRoundTrip();
    void testAnioRoundTrip();
    void testArtvRoundTrip();
    void testEcznRoundTrip();
    void testIpctRoundTrip();
    void testIpdsRoundTrip();
    void testMustRoundTrip();
    void testRevbRoundTrip();
    void testHdptRoundTrip();
    void testTermRoundTrip();
    void testMattRoundTrip();
    void testMovtRoundTrip();
    void testMuscRoundTrip();
};

void TestMissingRecords::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_missingrecords_log.txt"));
}

template <typename Rec>
static void roundTrip(quint32 typeCode, const Rec& src, Rec& loaded)
{
    QTemporaryFile tmpFile;
    tmpFile.open();
    QString path = tmpFile.fileName();
    tmpFile.close();

    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        ESMWriter writer;
        writer.setAuthor("Test");
        writer.save(file);
        RecHeader recHeader;
        recHeader.id = src.formId;
        writer.startRecord(typeCode, recHeader);
        src.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(path);
        reader.open();
        quint32 t = reader.readName();
        QCOMPARE(t, typeCode);
        loaded.load(reader, true);
    }
}

void TestMissingRecords::testRelaRoundTrip()
{
    RelaRecord rec;
    rec.editorId = QStringLiteral("TestRELA");
    rec.formId = 0x1001;
    rec.parentFormId = 0x1002;
    rec.childFormId = 0x1003;
    rec.rank = 3;
    rec.flags = 0x1;

    RelaRecord loaded;
    roundTrip<RelaRecord>('RELA', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testDebrRoundTrip()
{
    DebrRecord rec;
    rec.editorId = QStringLiteral("TestDEBR");
    rec.formId = 0x2001;
    DebrisEntry e;
    e.modelPath = QStringLiteral("debris/test.nif");
    e.count = 2;
    e.scale = 50;
    e.flags = 1;
    rec.debris.push_back(e);

    DebrRecord loaded;
    roundTrip<DebrRecord>('DEBR', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testHazdRoundTrip()
{
    HazdRecord rec;
    rec.initComponents();
    rec.editorId = QStringLiteral("TestHAZD");
    rec.formId = 0x3001;
    rec.modelPath = QStringLiteral("fx/testhazard.nif");
    rec.limit = 4;
    rec.radius = 32.0f;
    rec.lifetime = 2.5f;
    rec.imageSpace = 0x3002;
    rec.target = 1;
    rec.flags = 2;

    HazdRecord loaded;
    roundTrip<HazdRecord>('HAZD', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testShouRoundTrip()
{
    ShouRecord rec;
    rec.initComponents();
    rec.editorId = QStringLiteral("TestSHOU");
    rec.formId = 0x4001;
    rec.fullName = QStringLiteral("Test Shout");
    ShoutWord w;
    w.wordFormId = 0x4002;
    w.spellFormId = 0x4003;
    w.recoveryTime = 1.0f;
    rec.words.push_back(w);

    ShouRecord loaded;
    roundTrip<ShouRecord>('SHOU', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testClfmRoundTrip()
{
    ClfmRecord rec;
    rec.editorId = QStringLiteral("TestCLFM");
    rec.formId = 0x5001;
    rec.colorRgba = 0xFF00FF80u;
    rec.flags = 0x1;

    ClfmRecord loaded;
    roundTrip<ClfmRecord>('CLFM', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testAnioRoundTrip()
{
    AnioRecord rec;
    rec.editorId = QStringLiteral("TestANIO");
    rec.formId = 0x6001;
    rec.modelPath = QStringLiteral("anims/object.nif");

    AnioRecord loaded;
    roundTrip<AnioRecord>('ANIO', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testArtvRoundTrip()
{
    ArtvRecord rec;
    rec.initComponents();
    rec.editorId = QStringLiteral("TestARTV");
    rec.formId = 0x7001;
    rec.modelPath = QStringLiteral("fx/art.nif");
    rec.category = 3;

    ArtvRecord loaded;
    roundTrip<ArtvRecord>('ARTV', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testEcznRoundTrip()
{
    EcznRecord rec;
    rec.editorId = QStringLiteral("TestECZN");
    rec.formId = 0x8001;
    rec.zoneFormId = 0x8002;
    rec.locationFormId = 0x8003;
    rec.unusedFormId = 0x8004;
    rec.flags = 1;

    EcznRecord loaded;
    roundTrip<EcznRecord>('ECZN', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testIpctRoundTrip()
{
    IpctRecord rec;
    rec.initComponents();
    rec.editorId = QStringLiteral("TestIPCT");
    rec.formId = 0x9001;
    rec.modelPath = QStringLiteral("impact/test.nif");
    rec.materialType = 1;
    rec.flags = 4;
    rec.effectFormId = 0x9002;

    IpctRecord loaded;
    roundTrip<IpctRecord>('IPCT', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testIpdsRoundTrip()
{
    IpdsRecord rec;
    rec.editorId = QStringLiteral("TestIPDS");
    rec.formId = 0xA001;
    rec.impactFormIds = { 0xA002, 0xA003, 0xA004 };

    IpdsRecord loaded;
    roundTrip<IpdsRecord>('IPDS', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testMustRoundTrip()
{
    MustRecord rec;
    rec.editorId = QStringLiteral("TestMUST");
    rec.formId = 0xB001;
    rec.musicFile = QStringLiteral("music/test.xwm");
    rec.flags = 2;

    MustRecord loaded;
    roundTrip<MustRecord>('MUST', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testRevbRoundTrip()
{
    RevbRecord rec;
    rec.editorId = QStringLiteral("TestREVB");
    rec.formId = 0xC001;
    rec.flags = 3;

    RevbRecord loaded;
    roundTrip<RevbRecord>('REVB', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testHdptRoundTrip()
{
    HdptRecord rec;
    rec.editorId = QStringLiteral("TestHDPT");
    rec.formId = 0xD001;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("Test Head Part", 15);
    rec.rawSubRecords.push_back(full);

    HdptRecord loaded;
    roundTrip<HdptRecord>('HDPT', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testTermRoundTrip()
{
    TermRecord rec;
    rec.editorId = QStringLiteral("TestTERM");
    rec.formId = 0xD002;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("Test Terminal", 14);
    rec.rawSubRecords.push_back(full);

    TermRecord loaded;
    roundTrip<TermRecord>('TERM', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testMattRoundTrip()
{
    MattRecord rec;
    rec.editorId = QStringLiteral("TestMATT");
    rec.formId = 0xD003;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    MattRecord loaded;
    roundTrip<MattRecord>('MATT', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testMovtRoundTrip()
{
    MovtRecord rec;
    rec.editorId = QStringLiteral("TestMOVT");
    rec.formId = 0xD004;
    RawSubRecord dnam;
    dnam.name = 'DNAM';
    dnam.data = QByteArray("\x00\x00\x80\x3F", 4);
    rec.rawSubRecords.push_back(dnam);

    MovtRecord loaded;
    roundTrip<MovtRecord>('MOVT', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testMuscRoundTrip()
{
    MuscRecord rec;
    rec.editorId = QStringLiteral("TestMUSC");
    rec.formId = 0xD005;
    RawSubRecord wnam;
    wnam.name = 'WNAM';
    wnam.data = QByteArray("music/test.xwm", 14);
    rec.rawSubRecords.push_back(wnam);

    MuscRecord loaded;
    roundTrip<MuscRecord>('MUSC', rec, loaded);
    QCOMPARE(loaded, rec);
}

QTEST_MAIN(TestMissingRecords)
#include "test_missingrecords.moc"
