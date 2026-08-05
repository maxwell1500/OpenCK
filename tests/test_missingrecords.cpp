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
#include "../../libs/files/esm/bpttrecord.hpp"
#include "../../libs/files/esm/camsrecord.hpp"
#include "../../libs/files/esm/chalrecord.hpp"
#include "../../libs/files/esm/ciftrecord.hpp"
#include "../../libs/files/esm/cndarecord.hpp"
#include "../../libs/files/esm/collrecord.hpp"
#include "../../libs/files/esm/cpthrecord.hpp"
#include "../../libs/files/esm/culkrecord.hpp"
#include "../../libs/files/esm/cur3record.hpp"
#include "../../libs/files/esm/curvrecord.hpp"
#include "../../libs/files/esm/dfobrecord.hpp"
#include "../../libs/files/esm/dmgtrecord.hpp"
#include "../../libs/files/esm/dobjrecord.hpp"
#include "../../libs/files/esm/efsrrecord.hpp"
#include "../../libs/files/esm/equprecord.hpp"
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
    void testBpttRoundTrip();
    void testCamsRoundTrip();
    void testChalRoundTrip();
    void testCiftRoundTrip();
    void testCndaRoundTrip();
    void testCollRoundTrip();
    void testCpthRoundTrip();
    void testCulkRoundTrip();
    void testCur3RoundTrip();
    void testCurvRoundTrip();
    void testDfobRoundTrip();
    void testDmgtRoundTrip();
    void testDobjRoundTrip();
    void testEfsrRoundTrip();
    void testEqupRoundTrip();
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

void TestMissingRecords::testBpttRoundTrip()
{
    BpttRecord rec;
    rec.editorId = QStringLiteral("TestBPTT");
    rec.formId = 0xE015;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    BpttRecord loaded;
    roundTrip<BpttRecord>('BPTT', rec, loaded);
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

void TestMissingRecords::testCiftRoundTrip()
{
    CiftRecord rec;
    rec.editorId = QStringLiteral("TestCIFT");
    rec.formId = 0xE018;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    CiftRecord loaded;
    roundTrip<CiftRecord>('CIFT', rec, loaded);
    QCOMPARE(loaded, rec);
}

void TestMissingRecords::testCndaRoundTrip()
{
    CndaRecord rec;
    rec.editorId = QStringLiteral("TestCNDA");
    rec.formId = 0xE019;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    CndaRecord loaded;
    roundTrip<CndaRecord>('CNDA', rec, loaded);
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

void TestMissingRecords::testCulkRoundTrip()
{
    CulkRecord rec;
    rec.editorId = QStringLiteral("TestCULK");
    rec.formId = 0xE01C;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    CulkRecord loaded;
    roundTrip<CulkRecord>('CULK', rec, loaded);
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

void TestMissingRecords::testEfsrRoundTrip()
{
    EfsrRecord rec;
    rec.editorId = QStringLiteral("TestEFSR");
    rec.formId = 0xE022;
    RawSubRecord data;
    data.name = 'DATA';
    data.data = QByteArray("\x01\x00\x00\x00", 4);
    rec.rawSubRecords.push_back(data);

    EfsrRecord loaded;
    roundTrip<EfsrRecord>('EFSR', rec, loaded);
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

QTEST_MAIN(TestMissingRecords)
#include "test_missingrecords.moc"
