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

QTEST_MAIN(TestMissingRecords)
#include "test_missingrecords.moc"
