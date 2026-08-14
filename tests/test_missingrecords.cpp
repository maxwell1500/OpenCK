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
#include "../../libs/files/esm/aactrecord.hpp"
#include "../../libs/files/esm/aamdrecord.hpp"
#include "../../libs/files/esm/aapdrecord.hpp"
#include "../../libs/files/esm/achrrecord.hpp"
#include "../../libs/files/esm/addnrecord.hpp"
#include "../../libs/files/esm/afferecord.hpp"
#include "../../libs/files/esm/ambsrecord.hpp"
#include "../../libs/files/esm/amdlrecord.hpp"
#include "../../libs/files/esm/aopfrecord.hpp"
#include "../../libs/files/esm/aopsrecord.hpp"
#include "../../libs/files/esm/aorurecord.hpp"
#include "../../libs/files/esm/armarecord.hpp"
#include "../../libs/files/esm/artorecord.hpp"
#include "../../libs/files/esm/aspcrecord.hpp"
#include "../../libs/files/esm/atmrecord.hpp"
#include "../../libs/files/esm/avmdrecord.hpp"
#include "../../libs/files/esm/biomrecord.hpp"
#include "../../libs/files/esm/bmmorecord.hpp"
#include "../../libs/files/esm/bmodrecord.hpp"
#include "../../libs/files/esm/bndsrecord.hpp"
#include "../../libs/files/esm/bptdrecord.hpp"
#include "../../libs/files/esm/camsrecord.hpp"
#include "../../libs/files/esm/chalrecord.hpp"
#include "../../libs/files/esm/cldfrecord.hpp"
#include "../../libs/files/esm/cndfrecord.hpp"
#include "../../libs/files/esm/collrecord.hpp"
#include "../../libs/files/esm/cpthrecord.hpp"
#include "../../libs/files/esm/dlbrrecord.hpp"
#include "../../libs/files/esm/cur3record.hpp"
#include "../../libs/files/esm/curvrecord.hpp"
#include "../../libs/files/esm/dfobrecord.hpp"
#include "../../libs/files/esm/dmgtrecord.hpp"
#include "../../libs/files/esm/dobjrecord.hpp"
#include "../../libs/files/esm/efsqrecord.hpp"
#include "../../libs/files/esm/equprecord.hpp"
#include "../../libs/files/esm/ffkwrecord.hpp"
#include "../../libs/files/esm/fogvrecord.hpp"
#include "../../libs/files/esm/forcrecord.hpp"
#include "../../libs/files/esm/fstprecord.hpp"
#include "../../libs/files/esm/fstsrecord.hpp"
#include "../../libs/files/esm/fxpdrecord.hpp"
#include "../../libs/files/esm/gbfmrecord.hpp"
#include "../../libs/files/esm/gbftrecord.hpp"
#include "../../libs/files/esm/gcvrrecord.hpp"
#include "../../libs/files/esm/imadrecord.hpp"
#include "../../libs/files/esm/innrrecord.hpp"
#include "../../libs/files/esm/iresrecord.hpp"
#include "../../libs/files/esm/kssmrecord.hpp"
#include "../../libs/files/esm/layrrecord.hpp"
#include "../../libs/files/esm/lensrecord.hpp"
#include "../../libs/files/esm/lgdirecord.hpp"
#include "../../libs/files/esm/lgtmrecord.hpp"
#include "../../libs/files/esm/lmswrecord.hpp"
#include "../../libs/files/esm/lvlbrecord.hpp"
#include "../../libs/files/esm/lvlnrecord.hpp"
#include "../../libs/files/esm/lvlprecord.hpp"
#include "../../libs/files/esm/lvscrecord.hpp"
#include "../../libs/files/esm/maamrecord.hpp"
#include "../../libs/files/esm/mrhprecord.hpp"
#include "../../libs/files/esm/mtptrecord.hpp"
#include "../../libs/files/esm/navirecord.hpp"
#include "../../libs/files/esm/nocmrecord.hpp"
#include "../../libs/files/esm/omodrecord.hpp"
#include "../../libs/files/esm/oswprecord.hpp"
#include "../../libs/files/esm/ovisrecord.hpp"
#include "../../libs/files/esm/pcbnrecord.hpp"
#include "../../libs/files/esm/pccnrecord.hpp"
#include "../../libs/files/esm/pcmtrecord.hpp"
#include "../../libs/files/esm/pdclrecord.hpp"
#include "../../libs/files/esm/pgrerecord.hpp"
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
    void testAactRoundTrip();
    void testAamdRoundTrip();
    void testAapdRoundTrip();
    void testAchrRoundTrip();
    void testAddnRoundTrip();
    void testAffeRoundTrip();
    void testAmbsRoundTrip();
    void testAmdlRoundTrip();
    void testAopfRoundTrip();
    void testAopsRoundTrip();
    void testAoruRoundTrip();
    void testArmaRoundTrip();
    void testArtoRoundTrip();
    void testAspcRoundTrip();
    void testAtmoRoundTrip();
    void testAvmdRoundTrip();
    void testBiomRoundTrip();
    void testBmmoRoundTrip();
    void testBmodRoundTrip();
    void testBndsRoundTrip();
    void testBptdRoundTrip();
    void testCamsRoundTrip();
    void testChalRoundTrip();
    void testCldfRoundTrip();
    void testCndfRoundTrip();
    void testCollRoundTrip();
    void testCpthRoundTrip();
    void testDlbrRoundTrip();
    void testCur3RoundTrip();
    void testCurvRoundTrip();
    void testDfobRoundTrip();
    void testDmgtRoundTrip();
    void testDobjRoundTrip();
    void testEfsqRoundTrip();
    void testEqupRoundTrip();
    void testFfkwRoundTrip();
    void testFogvRoundTrip();
    void testForcRoundTrip();
    void testFstpRoundTrip();
    void testFstsRoundTrip();
    void testFxpdRoundTrip();
    void testGbfmRoundTrip();
    void testGbftRoundTrip();
    void testGcvrRoundTrip();
    void testImadRoundTrip();
    void testInnrRoundTrip();
    void testIresRoundTrip();
    void testKssmRoundTrip();
    void testLayrRoundTrip();
    void testLensRoundTrip();
    void testLgdiRoundTrip();
    void testLgtmRoundTrip();
    void testLmswRoundTrip();
    void testLvlbRoundTrip();
    void testLvlnRoundTrip();
    void testLvlpRoundTrip();
    void testLvscRoundTrip();
    void testMaamRoundTrip();
    void testMrhpRoundTrip();
    void testMtptRoundTrip();
    void testNaviRoundTrip();
    void testNocmRoundTrip();
    void testOmodRoundTrip();
    void testOswpRoundTrip();
    void testOvisRoundTrip();
    void testPcbnRoundTrip();
    void testPccnRoundTrip();
    void testPcmtRoundTrip();
    void testPdclRoundTrip();
    void testPgreRoundTrip();
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
    // DATA round-trips byte-exact as the raw payload (flags survive inside
    // it), so compare fields rather than whole-record equality.
    QCOMPARE(loaded.editorId, rec.editorId);
    QCOMPARE(loaded.formId, rec.formId);
    QCOMPARE(loaded.flags, rec.flags);
    QVERIFY(!loaded.data.isEmpty());
    QCOMPARE(loaded.data.size(), 40);
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

void TestMissingRecords::testAactRoundTrip()
{
    AactRecord rec;
    rec.editorId = QStringLiteral("TestAACT");
    rec.formId = 0xE001;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    AactRecord loaded;
    roundTrip<AactRecord>('AACT', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testAamdRoundTrip()
{
    AamdRecord rec;
    rec.editorId = QStringLiteral("TestAAMD");
    rec.formId = 0xE002;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    AamdRecord loaded;
    roundTrip<AamdRecord>('AAMD', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testAapdRoundTrip()
{
    AapdRecord rec;
    rec.editorId = QStringLiteral("TestAAPD");
    rec.formId = 0xE003;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    AapdRecord loaded;
    roundTrip<AapdRecord>('AAPD', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testAchrRoundTrip()
{
    AchrRecord rec;
    rec.editorId = QStringLiteral("TestACHR");
    rec.formId = 0xE004;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    AchrRecord loaded;
    roundTrip<AchrRecord>('ACHR', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testAddnRoundTrip()
{
    AddnRecord rec;
    rec.editorId = QStringLiteral("TestADDN");
    rec.formId = 0xE005;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    AddnRecord loaded;
    roundTrip<AddnRecord>('ADDN', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testAffeRoundTrip()
{
    AffeRecord rec;
    rec.editorId = QStringLiteral("TestAFFE");
    rec.formId = 0xE006;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    AffeRecord loaded;
    roundTrip<AffeRecord>('AFFE', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testAmbsRoundTrip()
{
    AmbsRecord rec;
    rec.editorId = QStringLiteral("TestAMBS");
    rec.formId = 0xE007;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    AmbsRecord loaded;
    roundTrip<AmbsRecord>('AMBS', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testAmdlRoundTrip()
{
    AmdlRecord rec;
    rec.editorId = QStringLiteral("TestAMDL");
    rec.formId = 0xE008;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    AmdlRecord loaded;
    roundTrip<AmdlRecord>('AMDL', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testAopfRoundTrip()
{
    AopfRecord rec;
    rec.editorId = QStringLiteral("TestAOPF");
    rec.formId = 0xE009;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    AopfRecord loaded;
    roundTrip<AopfRecord>('AOPF', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testAopsRoundTrip()
{
    AopsRecord rec;
    rec.editorId = QStringLiteral("TestAOPS");
    rec.formId = 0xE00A;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    AopsRecord loaded;
    roundTrip<AopsRecord>('AOPS', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testAoruRoundTrip()
{
    AoruRecord rec;
    rec.editorId = QStringLiteral("TestAORU");
    rec.formId = 0xE00B;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    AoruRecord loaded;
    roundTrip<AoruRecord>('AORU', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testArmaRoundTrip()
{
    ArmaRecord rec;
    rec.editorId = QStringLiteral("TestARMA");
    rec.formId = 0xE00C;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    ArmaRecord loaded;
    roundTrip<ArmaRecord>('ARMA', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testArtoRoundTrip()
{
    ArtoRecord rec;
    rec.editorId = QStringLiteral("TestARTO");
    rec.formId = 0xE00D;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    ArtoRecord loaded;
    roundTrip<ArtoRecord>('ARTO', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testAspcRoundTrip()
{
    AspcRecord rec;
    rec.editorId = QStringLiteral("TestASPC");
    rec.formId = 0xE00E;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    AspcRecord loaded;
    roundTrip<AspcRecord>('ASPC', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testAtmoRoundTrip()
{
    AtmoRecord rec;
    rec.editorId = QStringLiteral("TestATMO");
    rec.formId = 0xE00F;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    AtmoRecord loaded;
    roundTrip<AtmoRecord>('ATMO', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testAvmdRoundTrip()
{
    AvmdRecord rec;
    rec.editorId = QStringLiteral("TestAVMD");
    rec.formId = 0xE010;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    AvmdRecord loaded;
    roundTrip<AvmdRecord>('AVMD', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testBiomRoundTrip()
{
    BiomRecord rec;
    rec.editorId = QStringLiteral("TestBIOM");
    rec.formId = 0xE011;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    BiomRecord loaded;
    roundTrip<BiomRecord>('BIOM', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testBmmoRoundTrip()
{
    BmmoRecord rec;
    rec.editorId = QStringLiteral("TestBMMO");
    rec.formId = 0xE012;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    BmmoRecord loaded;
    roundTrip<BmmoRecord>('BMMO', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testBmodRoundTrip()
{
    BmodRecord rec;
    rec.editorId = QStringLiteral("TestBMOD");
    rec.formId = 0xE013;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    BmodRecord loaded;
    roundTrip<BmodRecord>('BMOD', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testBndsRoundTrip()
{
    BndsRecord rec;
    rec.editorId = QStringLiteral("TestBNDS");
    rec.formId = 0xE014;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    BndsRecord loaded;
    roundTrip<BndsRecord>('BNDS', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testBptdRoundTrip()
{
    BptdRecord rec;
    rec.editorId = QStringLiteral("TestBPTD");
    rec.formId = 0xE015;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    BptdRecord loaded;
    roundTrip<BptdRecord>('BPTD', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testCamsRoundTrip()
{
    CamsRecord rec;
    rec.editorId = QStringLiteral("TestCAMS");
    rec.formId = 0xE016;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    CamsRecord loaded;
    roundTrip<CamsRecord>('CAMS', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testChalRoundTrip()
{
    ChalRecord rec;
    rec.editorId = QStringLiteral("TestCHAL");
    rec.formId = 0xE017;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    ChalRecord loaded;
    roundTrip<ChalRecord>('CHAL', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testCldfRoundTrip()
{
    CldfRecord rec;
    rec.editorId = QStringLiteral("TestCLDF");
    rec.formId = 0xE018;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    CldfRecord loaded;
    roundTrip<CldfRecord>('CLDF', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testCndfRoundTrip()
{
    CndfRecord rec;
    rec.editorId = QStringLiteral("TestCNDF");
    rec.formId = 0xE019;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    CndfRecord loaded;
    roundTrip<CndfRecord>('CNDF', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testCollRoundTrip()
{
    CollRecord rec;
    rec.editorId = QStringLiteral("TestCOLL");
    rec.formId = 0xE01A;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    CollRecord loaded;
    roundTrip<CollRecord>('COLL', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testCpthRoundTrip()
{
    CpthRecord rec;
    rec.editorId = QStringLiteral("TestCPTH");
    rec.formId = 0xE01B;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    CpthRecord loaded;
    roundTrip<CpthRecord>('CPTH', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testDlbrRoundTrip()
{
    DlbrRecord rec;
    rec.editorId = QStringLiteral("TestDLBR");
    rec.formId = 0xE01C;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    DlbrRecord loaded;
    roundTrip<DlbrRecord>('DLBR', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testCur3RoundTrip()
{
    Cur3Record rec;
    rec.editorId = QStringLiteral("TestCUR3");
    rec.formId = 0xE01D;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    Cur3Record loaded;
    roundTrip<Cur3Record>('CUR3', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testCurvRoundTrip()
{
    CurvRecord rec;
    rec.editorId = QStringLiteral("TestCURV");
    rec.formId = 0xE01E;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    CurvRecord loaded;
    roundTrip<CurvRecord>('CURV', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testDfobRoundTrip()
{
    DfobRecord rec;
    rec.editorId = QStringLiteral("TestDFOB");
    rec.formId = 0xE01F;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    DfobRecord loaded;
    roundTrip<DfobRecord>('DFOB', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testDmgtRoundTrip()
{
    DmgtRecord rec;
    rec.editorId = QStringLiteral("TestDMGT");
    rec.formId = 0xE020;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    DmgtRecord loaded;
    roundTrip<DmgtRecord>('DMGT', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testDobjRoundTrip()
{
    DobjRecord rec;
    rec.editorId = QStringLiteral("TestDOBJ");
    rec.formId = 0xE021;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    DobjRecord loaded;
    roundTrip<DobjRecord>('DOBJ', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testEfsqRoundTrip()
{
    EfsqRecord rec;
    rec.editorId = QStringLiteral("TestEFSQ");
    rec.formId = 0xE022;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    EfsqRecord loaded;
    roundTrip<EfsqRecord>('EFSQ', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testEqupRoundTrip()
{
    EqupRecord rec;
    rec.editorId = QStringLiteral("TestEQUP");
    rec.formId = 0xE023;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    EqupRecord loaded;
    roundTrip<EqupRecord>('EQUP', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testFfkwRoundTrip()
{
    FfkwRecord rec;
    rec.editorId = QStringLiteral("TestFFKW");
    rec.formId = 0xE001;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestFFKW");
    rec.rawSubRecords.push_back(full);

    FfkwRecord loaded;
    roundTrip<FfkwRecord>('FFKW', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testFogvRoundTrip()
{
    FogvRecord rec;
    rec.editorId = QStringLiteral("TestFOGV");
    rec.formId = 0xE002;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestFOGV");
    rec.rawSubRecords.push_back(full);

    FogvRecord loaded;
    roundTrip<FogvRecord>('FOGV', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testForcRoundTrip()
{
    ForcRecord rec;
    rec.editorId = QStringLiteral("TestFORC");
    rec.formId = 0xE003;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestFORC");
    rec.rawSubRecords.push_back(full);

    ForcRecord loaded;
    roundTrip<ForcRecord>('FORC', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testFstpRoundTrip()
{
    FstpRecord rec;
    rec.editorId = QStringLiteral("TestFSTP");
    rec.formId = 0xE004;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestFSTP");
    rec.rawSubRecords.push_back(full);

    FstpRecord loaded;
    roundTrip<FstpRecord>('FSTP', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testFstsRoundTrip()
{
    FstsRecord rec;
    rec.editorId = QStringLiteral("TestFSTS");
    rec.formId = 0xE005;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestFSTS");
    rec.rawSubRecords.push_back(full);

    FstsRecord loaded;
    roundTrip<FstsRecord>('FSTS', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testFxpdRoundTrip()
{
    FxpdRecord rec;
    rec.editorId = QStringLiteral("TestFXPD");
    rec.formId = 0xE006;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestFXPD");
    rec.rawSubRecords.push_back(full);

    FxpdRecord loaded;
    roundTrip<FxpdRecord>('FXPD', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testGbfmRoundTrip()
{
    GbfmRecord rec;
    rec.editorId = QStringLiteral("TestGBFM");
    rec.formId = 0xE007;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestGBFM");
    rec.rawSubRecords.push_back(full);

    GbfmRecord loaded;
    roundTrip<GbfmRecord>('GBFM', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testGbftRoundTrip()
{
    GbftRecord rec;
    rec.editorId = QStringLiteral("TestGBFT");
    rec.formId = 0xE008;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestGBFT");
    rec.rawSubRecords.push_back(full);

    GbftRecord loaded;
    roundTrip<GbftRecord>('GBFT', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testGcvrRoundTrip()
{
    GcvrRecord rec;
    rec.editorId = QStringLiteral("TestGCVR");
    rec.formId = 0xE009;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestGCVR");
    rec.rawSubRecords.push_back(full);

    GcvrRecord loaded;
    roundTrip<GcvrRecord>('GCVR', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testImadRoundTrip()
{
    ImadRecord rec;
    rec.editorId = QStringLiteral("TestIMAD");
    rec.formId = 0xE00A;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestIMAD");
    rec.rawSubRecords.push_back(full);

    ImadRecord loaded;
    roundTrip<ImadRecord>('IMAD', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testInnrRoundTrip()
{
    InnrRecord rec;
    rec.editorId = QStringLiteral("TestINNR");
    rec.formId = 0xE00B;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestINNR");
    rec.rawSubRecords.push_back(full);

    InnrRecord loaded;
    roundTrip<InnrRecord>('INNR', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testIresRoundTrip()
{
    IresRecord rec;
    rec.editorId = QStringLiteral("TestIRES");
    rec.formId = 0xE00C;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestIRES");
    rec.rawSubRecords.push_back(full);

    IresRecord loaded;
    roundTrip<IresRecord>('IRES', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testKssmRoundTrip()
{
    KssmRecord rec;
    rec.editorId = QStringLiteral("TestKSSM");
    rec.formId = 0xE00D;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestKSSM");
    rec.rawSubRecords.push_back(full);

    KssmRecord loaded;
    roundTrip<KssmRecord>('KSSM', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testLayrRoundTrip()
{
    LayrRecord rec;
    rec.editorId = QStringLiteral("TestLAYR");
    rec.formId = 0xE00E;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestLAYR");
    rec.rawSubRecords.push_back(full);

    LayrRecord loaded;
    roundTrip<LayrRecord>('LAYR', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testLensRoundTrip()
{
    LensRecord rec;
    rec.editorId = QStringLiteral("TestLENS");
    rec.formId = 0xE00F;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestLENS");
    rec.rawSubRecords.push_back(full);

    LensRecord loaded;
    roundTrip<LensRecord>('LENS', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testLgdiRoundTrip()
{
    LgdiRecord rec;
    rec.editorId = QStringLiteral("TestLGDI");
    rec.formId = 0xE010;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestLGDI");
    rec.rawSubRecords.push_back(full);

    LgdiRecord loaded;
    roundTrip<LgdiRecord>('LGDI', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testLgtmRoundTrip()
{
    LgtmRecord rec;
    rec.editorId = QStringLiteral("TestLGTM");
    rec.formId = 0xE011;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestLGTM");
    rec.rawSubRecords.push_back(full);

    LgtmRecord loaded;
    roundTrip<LgtmRecord>('LGTM', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testLmswRoundTrip()
{
    LmswRecord rec;
    rec.editorId = QStringLiteral("TestLMSW");
    rec.formId = 0xE012;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestLMSW");
    rec.rawSubRecords.push_back(full);

    LmswRecord loaded;
    roundTrip<LmswRecord>('LMSW', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testLvlbRoundTrip()
{
    LvlbRecord rec;
    rec.editorId = QStringLiteral("TestLVLB");
    rec.formId = 0xE013;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestLVLB");
    rec.rawSubRecords.push_back(full);

    LvlbRecord loaded;
    roundTrip<LvlbRecord>('LVLB', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testLvlnRoundTrip()
{
    LvlnRecord rec;
    rec.editorId = QStringLiteral("TestLVLN");
    rec.formId = 0xE014;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestLVLN");
    rec.rawSubRecords.push_back(full);

    LvlnRecord loaded;
    roundTrip<LvlnRecord>('LVLN', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testLvlpRoundTrip()
{
    LvlpRecord rec;
    rec.editorId = QStringLiteral("TestLVLP");
    rec.formId = 0xE015;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestLVLP");
    rec.rawSubRecords.push_back(full);

    LvlpRecord loaded;
    roundTrip<LvlpRecord>('LVLP', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testLvscRoundTrip()
{
    LvscRecord rec;
    rec.editorId = QStringLiteral("TestLVSC");
    rec.formId = 0xE016;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestLVSC");
    rec.rawSubRecords.push_back(full);

    LvscRecord loaded;
    roundTrip<LvscRecord>('LVSC', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testMaamRoundTrip()
{
    MaamRecord rec;
    rec.editorId = QStringLiteral("TestMAAM");
    rec.formId = 0xE017;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestMAAM");
    rec.rawSubRecords.push_back(full);

    MaamRecord loaded;
    roundTrip<MaamRecord>('MAAM', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testMrhpRoundTrip()
{
    MrhpRecord rec;
    rec.editorId = QStringLiteral("TestMRHP");
    rec.formId = 0xE018;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestMRHP");
    rec.rawSubRecords.push_back(full);

    MrhpRecord loaded;
    roundTrip<MrhpRecord>('MRPH', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testMtptRoundTrip()
{
    MtptRecord rec;
    rec.editorId = QStringLiteral("TestMTPT");
    rec.formId = 0xE019;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestMTPT");
    rec.rawSubRecords.push_back(full);

    MtptRecord loaded;
    roundTrip<MtptRecord>('MTPT', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testNaviRoundTrip()
{
    NaviRecord rec;
    rec.editorId = QStringLiteral("TestNAVI");
    rec.formId = 0xE01A;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestNAVI");
    rec.rawSubRecords.push_back(full);

    NaviRecord loaded;
    roundTrip<NaviRecord>('NAVI', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testNocmRoundTrip()
{
    NocmRecord rec;
    rec.editorId = QStringLiteral("TestNOCM");
    rec.formId = 0xE01B;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestNOCM");
    rec.rawSubRecords.push_back(full);

    NocmRecord loaded;
    roundTrip<NocmRecord>('NOCM', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testOmodRoundTrip()
{
    OmodRecord rec;
    rec.editorId = QStringLiteral("TestOMOD");
    rec.formId = 0xE01C;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestOMOD");
    rec.rawSubRecords.push_back(full);

    OmodRecord loaded;
    roundTrip<OmodRecord>('OMOD', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testOswpRoundTrip()
{
    OswpRecord rec;
    rec.editorId = QStringLiteral("TestOSWP");
    rec.formId = 0xE01D;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestOSWP");
    rec.rawSubRecords.push_back(full);

    OswpRecord loaded;
    roundTrip<OswpRecord>('OSWP', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testOvisRoundTrip()
{
    OvisRecord rec;
    rec.editorId = QStringLiteral("TestOVIS");
    rec.formId = 0xE01E;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestOVIS");
    rec.rawSubRecords.push_back(full);

    OvisRecord loaded;
    roundTrip<OvisRecord>('OVIS', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testPcbnRoundTrip()
{
    PcbnRecord rec;
    rec.editorId = QStringLiteral("TestPCBN");
    rec.formId = 0xE01F;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestPCBN");
    rec.rawSubRecords.push_back(full);

    PcbnRecord loaded;
    roundTrip<PcbnRecord>('PCBN', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testPccnRoundTrip()
{
    PccnRecord rec;
    rec.editorId = QStringLiteral("TestPCCN");
    rec.formId = 0xE020;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestPCCN");
    rec.rawSubRecords.push_back(full);

    PccnRecord loaded;
    roundTrip<PccnRecord>('PCCN', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testPcmtRoundTrip()
{
    PcmtRecord rec;
    rec.editorId = QStringLiteral("TestPCMT");
    rec.formId = 0xE021;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestPCMT");
    rec.rawSubRecords.push_back(full);

    PcmtRecord loaded;
    roundTrip<PcmtRecord>('PCMT', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testPdclRoundTrip()
{
    PdclRecord rec;
    rec.editorId = QStringLiteral("TestPDCL");
    rec.formId = 0xE022;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestPDCL");
    rec.rawSubRecords.push_back(full);

    PdclRecord loaded;
    roundTrip<PdclRecord>('PDCL', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testPgreRoundTrip()
{
    PgreRecord rec;
    rec.editorId = QStringLiteral("TestPGRE");
    rec.formId = 0xE023;
    RawSubRecord full;
    full.name = 'FULL';
    full.data = QByteArray("TestPGRE");
    rec.rawSubRecords.push_back(full);

    PgreRecord loaded;
    roundTrip<PgreRecord>('PGRE', rec, loaded);
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
