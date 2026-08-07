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
#include "../../libs/files/esm/phzdrecord.hpp"
#include "../../libs/files/esm/pkinrecord.hpp"
#include "../../libs/files/esm/pmftrecord.hpp"
#include "../../libs/files/esm/psdcrecord.hpp"
#include "../../libs/files/esm/ptstrecord.hpp"
#include "../../libs/files/esm/rfgprecord.hpp"
#include "../../libs/files/esm/rsgdrecord.hpp"
#include "../../libs/files/esm/rspjrecord.hpp"
#include "../../libs/files/esm/sdltrecord.hpp"
#include "../../libs/files/esm/sechrecord.hpp"
#include "../../libs/files/esm/sfbkrecord.hpp"
#include "../../libs/files/esm/sfpcrecord.hpp"
#include "../../libs/files/esm/sfptrecord.hpp"
#include "../../libs/files/esm/sftrrecord.hpp"
#include "../../libs/files/esm/smbnrecord.hpp"
#include "../../libs/files/esm/smenrecord.hpp"
#include "../../libs/files/esm/spchrecord.hpp"
#include "../../libs/files/esm/stagrecord.hpp"
#include "../../libs/files/esm/stbhrecord.hpp"
#include "../../libs/files/esm/stdtrecord.hpp"
#include "../../libs/files/esm/stmprecord.hpp"
#include "../../libs/files/esm/stndrecord.hpp"
#include "../../libs/files/esm/sunprecord.hpp"
#include "../../libs/files/esm/tmlmrecord.hpp"
#include "../../libs/files/esm/toddrecord.hpp"
#include "../../libs/files/esm/travrecord.hpp"
#include "../../libs/files/esm/trnsrecord.hpp"
#include "../../libs/files/esm/volirecord.hpp"
#include "../../libs/files/esm/vtyprecord.hpp"
#include "../../libs/files/esm/wbarrecord.hpp"
#include "../../libs/files/esm/wkmfrecord.hpp"
#include "../../libs/files/esm/wthsrecord.hpp"
#include "../../libs/files/esm/wwedrecord.hpp"
#include "../../libs/files/esm/zoomrecord.hpp"
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
    void testPhzdRoundTrip();
    void testPkinRoundTrip();
    void testPmftRoundTrip();
    void testPsdcRoundTrip();
    void testPtstRoundTrip();
    void testRfgpRoundTrip();
    void testRsgdRoundTrip();
    void testRspjRoundTrip();
    void testSdltRoundTrip();
    void testSechRoundTrip();
    void testSfbkRoundTrip();
    void testSfpcRoundTrip();
    void testSfptRoundTrip();
    void testSftrRoundTrip();
    void testSmbnRoundTrip();
    void testSmenRoundTrip();
    void testSpchRoundTrip();
    void testStagRoundTrip();
    void testStbhRoundTrip();
    void testStdtRoundTrip();
    void testStmpRoundTrip();
    void testStndRoundTrip();
    void testSunpRoundTrip();
    void testTmlmRoundTrip();
    void testToddRoundTrip();
    void testTravRoundTrip();
    void testTrnsRoundTrip();
    void testVoliRoundTrip();
    void testVtypRoundTrip();
    void testWbarRoundTrip();
    void testWkmfRoundTrip();
    void testWthsRoundTrip();
    void testWwedRoundTrip();
    void testZoomRoundTrip();
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

void TestMissingRecords::testPhzdRoundTrip()
{
    PhzdRecord rec;
    rec.editorId = QStringLiteral("TestPHZD");
    rec.formId = 0xD006;

    PhzdRecord loaded;
    roundTrip<PhzdRecord>('PHZD', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testPkinRoundTrip()
{
    PkinRecord rec;
    rec.editorId = QStringLiteral("TestPKIN");
    rec.formId = 0xD007;

    PkinRecord loaded;
    roundTrip<PkinRecord>('PKIN', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testPmftRoundTrip()
{
    PmftRecord rec;
    rec.editorId = QStringLiteral("TestPMFT");
    rec.formId = 0xD008;

    PmftRecord loaded;
    roundTrip<PmftRecord>('PMFT', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testPsdcRoundTrip()
{
    PsdcRecord rec;
    rec.editorId = QStringLiteral("TestPSDC");
    rec.formId = 0xD009;

    PsdcRecord loaded;
    roundTrip<PsdcRecord>('PSDC', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testPtstRoundTrip()
{
    PtstRecord rec;
    rec.editorId = QStringLiteral("TestPTST");
    rec.formId = 0xD00A;

    PtstRecord loaded;
    roundTrip<PtstRecord>('PTST', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testRfgpRoundTrip()
{
    RfgpRecord rec;
    rec.editorId = QStringLiteral("TestRFGP");
    rec.formId = 0xD00B;

    RfgpRecord loaded;
    roundTrip<RfgpRecord>('RFGP', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testRsgdRoundTrip()
{
    RsgdRecord rec;
    rec.editorId = QStringLiteral("TestRSGD");
    rec.formId = 0xD00C;

    RsgdRecord loaded;
    roundTrip<RsgdRecord>('RSGD', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testRspjRoundTrip()
{
    RspjRecord rec;
    rec.editorId = QStringLiteral("TestRSPJ");
    rec.formId = 0xD00D;

    RspjRecord loaded;
    roundTrip<RspjRecord>('RSPJ', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testSdltRoundTrip()
{
    SdltRecord rec;
    rec.editorId = QStringLiteral("TestSDLT");
    rec.formId = 0xD00E;

    SdltRecord loaded;
    roundTrip<SdltRecord>('SDLT', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testSechRoundTrip()
{
    SechRecord rec;
    rec.editorId = QStringLiteral("TestSECH");
    rec.formId = 0xD00F;

    SechRecord loaded;
    roundTrip<SechRecord>('SECH', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testSfbkRoundTrip()
{
    SfbkRecord rec;
    rec.editorId = QStringLiteral("TestSFBK");
    rec.formId = 0xD010;

    SfbkRecord loaded;
    roundTrip<SfbkRecord>('SFBK', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testSfpcRoundTrip()
{
    SfpcRecord rec;
    rec.editorId = QStringLiteral("TestSFPC");
    rec.formId = 0xD011;

    SfpcRecord loaded;
    roundTrip<SfpcRecord>('SFPC', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testSfptRoundTrip()
{
    SfptRecord rec;
    rec.editorId = QStringLiteral("TestSFPT");
    rec.formId = 0xD012;

    SfptRecord loaded;
    roundTrip<SfptRecord>('SFPT', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testSftrRoundTrip()
{
    SftrRecord rec;
    rec.editorId = QStringLiteral("TestSFTR");
    rec.formId = 0xD013;

    SftrRecord loaded;
    roundTrip<SftrRecord>('SFTR', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testSmbnRoundTrip()
{
    SmbnRecord rec;
    rec.editorId = QStringLiteral("TestSMBN");
    rec.formId = 0xD014;

    SmbnRecord loaded;
    roundTrip<SmbnRecord>('SMBN', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testSmenRoundTrip()
{
    SmenRecord rec;
    rec.editorId = QStringLiteral("TestSMEN");
    rec.formId = 0xD015;

    SmenRecord loaded;
    roundTrip<SmenRecord>('SMEN', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testSpchRoundTrip()
{
    SpchRecord rec;
    rec.editorId = QStringLiteral("TestSPCH");
    rec.formId = 0xD016;

    SpchRecord loaded;
    roundTrip<SpchRecord>('SPCH', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testStagRoundTrip()
{
    StagRecord rec;
    rec.editorId = QStringLiteral("TestSTAG");
    rec.formId = 0xD017;

    StagRecord loaded;
    roundTrip<StagRecord>('STAG', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testStbhRoundTrip()
{
    StbhRecord rec;
    rec.editorId = QStringLiteral("TestSTBH");
    rec.formId = 0xD018;

    StbhRecord loaded;
    roundTrip<StbhRecord>('STBH', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testStdtRoundTrip()
{
    StdtRecord rec;
    rec.editorId = QStringLiteral("TestSTDT");
    rec.formId = 0xD019;

    StdtRecord loaded;
    roundTrip<StdtRecord>('STDT', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testStmpRoundTrip()
{
    StmpRecord rec;
    rec.editorId = QStringLiteral("TestSTMP");
    rec.formId = 0xD01A;

    StmpRecord loaded;
    roundTrip<StmpRecord>('STMP', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testStndRoundTrip()
{
    StndRecord rec;
    rec.editorId = QStringLiteral("TestSTND");
    rec.formId = 0xD01B;

    StndRecord loaded;
    roundTrip<StndRecord>('STND', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testSunpRoundTrip()
{
    SunpRecord rec;
    rec.editorId = QStringLiteral("TestSUNP");
    rec.formId = 0xD01C;

    SunpRecord loaded;
    roundTrip<SunpRecord>('SUNP', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testTmlmRoundTrip()
{
    TmlmRecord rec;
    rec.editorId = QStringLiteral("TestTMLM");
    rec.formId = 0xD01D;

    TmlmRecord loaded;
    roundTrip<TmlmRecord>('TMLM', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testToddRoundTrip()
{
    ToddRecord rec;
    rec.editorId = QStringLiteral("TestTODD");
    rec.formId = 0xD01E;

    ToddRecord loaded;
    roundTrip<ToddRecord>('TODD', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testTravRoundTrip()
{
    TravRecord rec;
    rec.editorId = QStringLiteral("TestTRAV");
    rec.formId = 0xD01F;

    TravRecord loaded;
    roundTrip<TravRecord>('TRAV', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testTrnsRoundTrip()
{
    TrnsRecord rec;
    rec.editorId = QStringLiteral("TestTRNS");
    rec.formId = 0xD020;

    TrnsRecord loaded;
    roundTrip<TrnsRecord>('TRNS', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testVoliRoundTrip()
{
    VoliRecord rec;
    rec.editorId = QStringLiteral("TestVOLI");
    rec.formId = 0xD021;

    VoliRecord loaded;
    roundTrip<VoliRecord>('VOLI', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testVtypRoundTrip()
{
    VtypRecord rec;
    rec.editorId = QStringLiteral("TestVTYP");
    rec.formId = 0xD022;

    VtypRecord loaded;
    roundTrip<VtypRecord>('VTYP', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testWbarRoundTrip()
{
    WbarRecord rec;
    rec.editorId = QStringLiteral("TestWBAR");
    rec.formId = 0xD023;

    WbarRecord loaded;
    roundTrip<WbarRecord>('WBAR', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testWkmfRoundTrip()
{
    WkmfRecord rec;
    rec.editorId = QStringLiteral("TestWKMF");
    rec.formId = 0xD024;

    WkmfRecord loaded;
    roundTrip<WkmfRecord>('WKMF', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testWthsRoundTrip()
{
    WthsRecord rec;
    rec.editorId = QStringLiteral("TestWTHS");
    rec.formId = 0xD025;

    WthsRecord loaded;
    roundTrip<WthsRecord>('WTHS', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testWwedRoundTrip()
{
    WwedRecord rec;
    rec.editorId = QStringLiteral("TestWWED");
    rec.formId = 0xD026;

    WwedRecord loaded;
    roundTrip<WwedRecord>('WWED', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testZoomRoundTrip()
{
    ZoomRecord rec;
    rec.editorId = QStringLiteral("TestZOOM");
    rec.formId = 0xD027;

    ZoomRecord loaded;
    roundTrip<ZoomRecord>('ZOOM', rec, loaded);
    QCOMPARE(loaded, rec);
}

QTEST_MAIN(TestMissingRecords)
#include "test_missingrecords.moc"
