#include <QTest>
#include <QBuffer>
#include <QTemporaryFile>
#include <QFileInfo>
#include <cmath>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "pndrecord.hpp"
#include "common.hpp"
#include "logger.hpp"
#include "model/tools/planetcodec.hpp"

// PlanetDefinition <-> PndRecord binary codec (REMAINING.md §3.8). The pure
// slots pin the mapping contract; testRealPndtThroughCodec validates it
// against the real Starfield.esm (same gating as test_pndrecord).
class TestPlanetCodec : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testDefToRecordNew();
    void testDefToRecordOverBase();
    void testNewRecordSaveEmitsEdidAnam();
    void testRealPndtThroughCodec();
};

void TestPlanetCodec::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_planetcodec_log.txt"));
}

void TestPlanetCodec::testDefToRecordNew()
{
    PlanetDefinition def;
    def.editorId = QStringLiteral("TestPlanetData");
    def.starSystem = QStringLiteral("Test System");
    def.temperature = QStringLiteral("21");

    const PndRecord rec = PlanetCodec::toRecord(def);
    QCOMPARE(rec.editorId, QStringLiteral("TestPlanetData"));
    QCOMPARE(rec.starSystem, QStringLiteral("Test System"));
    QCOMPARE(rec.temperature, 21.0f);
    QCOMPARE(rec.density, 1.0f);
    QCOMPARE(rec.phase, 1.0f);
    QCOMPARE(rec.resources, quint32(0));
    QVERIFY(rec.rawSubRecords.isEmpty());
    QCOMPARE(rec.mOrder, QVector<quint32>({(NAME)'EDID', (NAME)'FNAM', (NAME)'ANAM',
                                           (NAME)'TEMP', (NAME)'DENS', (NAME)'PHLA',
                                           (NAME)'RSCS'}));
    QVERIFY(!PlanetCodec::hasUndecodedData(rec));

    // A display label (no leading number) encodes as TEMP 0.0.
    PlanetDefinition labeled = def;
    labeled.temperature = QStringLiteral("Temperate");
    QCOMPARE(PlanetCodec::toRecord(labeled).temperature, 0.0f);
}

void TestPlanetCodec::testDefToRecordOverBase()
{
    PndRecord base;
    base.editorId = QStringLiteral("MarsPlanetData");
    base.formId = 0x1234;
    base.flags = 7;
    base.starSystem = QStringLiteral("Mars");
    base.temperature = -5.0f;
    base.density = 3.93f;
    base.phase = 176.0f;
    base.resources = 0xe0e99e4;
    RawSubRecord raw;
    raw.name = (NAME)'FULL';
    raw.data = QByteArray("Mars");
    base.rawSubRecords.append(raw);
    base.mOrder.append((NAME)'EDID');
    base.mOrder.append((NAME)'FULL');

    // Editing the ids round-trips everything else verbatim.
    PlanetDefinition def = PlanetCodec::fromRecord(base);
    QCOMPARE(def.editorId, QStringLiteral("MarsPlanetData"));
    QCOMPARE(def.starSystem, QStringLiteral("Mars"));
    def.editorId = QStringLiteral("MarsCopyData");

    const PndRecord edited = PlanetCodec::toRecord(def, &base);
    QCOMPARE(edited.editorId, QStringLiteral("MarsCopyData"));
    QCOMPARE(edited.formId, quint32(0x1234));
    QCOMPARE(edited.flags, quint32(7));
    QCOMPARE(edited.temperature, -5.0f);
    QCOMPARE(edited.density, 3.93f);
    QCOMPARE(edited.phase, 176.0f);
    QCOMPARE(edited.resources, quint32(0xe0e99e4));
    QCOMPARE(edited.rawSubRecords.size(), 1);
    QCOMPARE(edited.mOrder.size(), 2);
    QVERIFY(PlanetCodec::hasUndecodedData(edited));

    // A display label keeps the base record's measured TEMP.
    PlanetDefinition relabeled = def;
    relabeled.temperature = QStringLiteral("Cold");
    QCOMPARE(PlanetCodec::toRecord(relabeled, &base).temperature, -5.0f);

    // A numeric string overrides it.
    PlanetDefinition remeasured = def;
    remeasured.temperature = QStringLiteral("-12");
    QCOMPARE(PlanetCodec::toRecord(remeasured, &base).temperature, -12.0f);
}

void TestPlanetCodec::testNewRecordSaveEmitsEdidAnam()
{
    PlanetDefinition def;
    def.editorId = QStringLiteral("NewPlanetData");
    def.starSystem = QStringLiteral("New System");
    def.temperature = QStringLiteral("10");
    const PndRecord rec = PlanetCodec::toRecord(def);

    QTemporaryFile written;
    QVERIFY(written.open());
    {
        ESMWriter writer;
        writer.setAuthor("Test");
        writer.save(written);
        RecHeader rh;
        rh.id = rec.formId;
        writer.startRecord('PNDT', rh);
        rec.save(writer);
        writer.endRecord();
        writer.close();
    }
    written.close();

    ESMReader check(written.fileName());
    check.open();
    bool found = false;
    while (check.isLeft())
    {
        NAME name = check.readName();
        if (name == 0) break;
        if (name == (NAME)'GRUP') { check.skipGrupHeader(); continue; }
        if (name != (NAME)'PNDT') { check.skipRecord(); continue; }
        PndRecord back;
        back.load(check, true);
        found = true;
        QCOMPARE(back.editorId, QStringLiteral("NewPlanetData"));
        QCOMPARE(back.starSystem, QStringLiteral("New System"));
        QCOMPARE(back.temperature, 10.0f);
        break;
    }
    QVERIFY(found);
}

void TestPlanetCodec::testRealPndtThroughCodec()
{
    const QString filePath = qEnvironmentVariable("OPENCK_DATA_DIR", QStringLiteral("C:/XboxGames/Starfield/Content/Data")) + "/Starfield.esm";
    if (!QFileInfo::exists(filePath)) QSKIP("Starfield.esm not found");

    ESMReader reader(filePath);
    reader.open();

    int checked = 0;
    while (reader.isLeft() && checked < 10)
    {
        NAME name = 0;
        try {
            name = reader.readName();
        } catch (...) {
            break;
        }
        if (name == 0) break;
        if (name == (NAME)'GRUP') { reader.skipGrupHeader(); continue; }
        if (name != (NAME)'PNDT') { reader.skipRecord(); continue; }

        PndRecord real;
        real.load(reader, true);

        // Through the editor model and back over the original as base: every
        // typed field and every raw byte must survive.
        const PlanetDefinition def = PlanetCodec::fromRecord(real);
        QCOMPARE(def.editorId, real.editorId);
        QCOMPARE(def.starSystem, real.starSystem);

        const PndRecord back = PlanetCodec::toRecord(def, &real);
        QCOMPARE(back.editorId, real.editorId);
        QCOMPARE(back.formId, real.formId);
        QCOMPARE(back.flags, real.flags);
        QCOMPARE(back.starSystem, real.starSystem);
        QVERIFY(std::abs(back.temperature - real.temperature) < 0.001f);
        QVERIFY(std::abs(back.density - real.density) < 0.0001f);
        QVERIFY(std::abs(back.phase - real.phase) < 0.0001f);
        QCOMPARE(back.resources, real.resources);
        QCOMPARE(back.rawSubRecords.size(), real.rawSubRecords.size());
        QCOMPARE(back.mOrder, real.mOrder);
        for (int i = 0; i < real.rawSubRecords.size(); ++i)
        {
            QCOMPARE(back.rawSubRecords[i].name, real.rawSubRecords[i].name);
            QCOMPARE(back.rawSubRecords[i].data, real.rawSubRecords[i].data);
        }
        // Real records always carry undecoded subrecords (keywords, model…).
        QVERIFY(PlanetCodec::hasUndecodedData(back));
        ++checked;
    }
    QCOMPARE(checked, 10);
}

QTEST_MAIN(TestPlanetCodec)
#include "test_planetcodec.moc"
