#include <QTest>
#include <QTemporaryFile>
#include <QFileInfo>
#include <cmath>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "worldspacerecord.hpp"
#include "common.hpp"
#include "logger.hpp"

// Validates the WorldspaceRecord binary encoder against the real Skyrim SE
// WRLD layout (map data, climate/water/lighting, LOD fields, raw blocks).
// Requires the user's Skyrim SE install; not registered with CTest by default.
class TestWorldspaceRecord : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testRoundTripRealRecords();
};

void TestWorldspaceRecord::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral(
        "C:/Users/max/AppData/Local/Temp/opencode/test_worldspacerecord_log.txt"));
}

static bool parseNext(ESMReader& reader, WorldspaceRecord& rec, int* scanned)
{
    while (reader.isLeft())
    {
        NAME name = 0;
        try {
            name = reader.readName();
        } catch (...) {
            return false;
        }
        if (name == 0) return false;
        if (name == (NAME)'GRUP') { reader.skipGrupHeader(); continue; }
        if (name == (NAME)'WRLD')
        {
            rec.load(reader, true);
            // Drain any leftover compressed buffer so the walk stays aligned.
            reader.skipRemainingRecord();
            if (scanned) ++(*scanned);
            return true;
        }
        reader.skipRecord();
        if (scanned) ++(*scanned);
    }
    return false;
}

void TestWorldspaceRecord::testRoundTripRealRecords()
{
    const QString filePath = QStringLiteral(
        "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data/Skyrim.esm");
    QVERIFY2(QFileInfo::exists(filePath), "Skyrim.esm not found");

    ESMReader reader(filePath);
    reader.open();

    int roundTripped = 0;
    int scanned = 0;
    int failures = 0;
    while (scanned < 30000000)
    {
        WorldspaceRecord rec;
        if (!parseNext(reader, rec, &scanned))
            break;
        if (roundTripped >= 20) break;

        QTemporaryFile written;
        QVERIFY(written.open());
        {
            ESMWriter writer;
            writer.setAuthor("Test");
            writer.save(written);
            RecHeader rh;
            rh.id = rec.formId;
            writer.startRecord('WRLD', rh);
            rec.save(writer);
            writer.endRecord();
            writer.close();
        }
        written.close();
        const QString writtenPath = written.fileName();

        ESMReader check(writtenPath);
        check.open();
        bool found = false;
        while (check.isLeft())
        {
            NAME name = check.readName();
            if (name == 0) break;
            if (name == (NAME)'GRUP') { check.skipGrupHeader(); continue; }
            if (name == (NAME)'WRLD')
            {
                WorldspaceRecord rt;
                rt.load(check, true);
                found = true;
                if (rec.editorId != rt.editorId ||
                    rec.formId != rt.formId ||
                    rec.name != rt.name ||
                    rec.waterType != rt.waterType ||
                    rec.climateId != rt.climateId ||
                    rec.lightingId != rt.lightingId ||
                    rec.mapWidth != rt.mapWidth ||
                    rec.mapHeight != rt.mapHeight ||
                    rec.mapNwX != rt.mapNwX ||
                    rec.mapNwY != rt.mapNwY ||
                    rec.mapSeX != rt.mapSeX ||
                    rec.mapSeY != rt.mapSeY ||
                    std::abs(rec.mapScale() - rt.mapScale()) > 0.001f ||
                    std::abs(rec.mapLodBias - rt.mapLodBias) > 0.001f ||
                    rec.onamData != rt.onamData ||
                    rec.dataFlags != rt.dataFlags ||
                    rec.dnamData != rt.dnamData ||
                    rec.rawSubRecords.size() != rt.rawSubRecords.size() ||
                    rec.mOrder != rt.mOrder)
                {
                    ++failures;
                    qWarning() << "Round-trip mismatch on " << rec.editorId;
                }
                for (int i = 0; i < rec.rawSubRecords.size(); ++i)
                {
                    if (rec.rawSubRecords[i].name != rt.rawSubRecords[i].name ||
                        rec.rawSubRecords[i].data != rt.rawSubRecords[i].data)
                    {
                        ++failures;
                        qWarning() << "Raw subrecord mismatch on " << rec.editorId
                                   << " at index " << i;
                    }
                }
                ++roundTripped;
                break;
            }
            check.skipRecord();
        }
    }

    qDebug() << "WRLD records round-tripped:" << roundTripped
             << "failures:" << failures;
    QVERIFY(roundTripped >= 20);
    QCOMPARE(failures, 0);
}

QTEST_MAIN(TestWorldspaceRecord)
#include "test_worldspacerecord.moc"
