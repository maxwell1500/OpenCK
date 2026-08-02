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

// Validates the PndRecord binary encoder against the real Starfield.esm.
// Requires the user's Starfield install; like test_starfieldesm this is not
// registered with CTest by default.
class TestPndRecord : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testRoundTripRealRecords();
};

void TestPndRecord::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_pndrecord_log.txt"));
}

static bool parseNext(ESMReader& reader, PndRecord& rec, int* recordsScanned)
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
        if (name == (NAME)'PNDT')
        {
            rec.load(reader, true);
            if (recordsScanned) ++(*recordsScanned);
            return true;
        }
        reader.skipRecord();
        if (recordsScanned) ++(*recordsScanned);
    }
    return false;
}

void TestPndRecord::testRoundTripRealRecords()
{
    const QString filePath = QStringLiteral("C:/XboxGames/Starfield/Content/Data/Starfield.esm");
    QVERIFY2(QFileInfo::exists(filePath), "Starfield.esm not found - set OPENCK_TEST_STARFIELD_ESM");

    ESMReader reader(filePath);
    reader.open();

    int roundTripped = 0;
    int scanned = 0;
    int failures = 0;
    while (scanned < 10000000)
    {
        PndRecord rec;
        if (!parseNext(reader, rec, &scanned))
            break;
        if (roundTripped >= 20) break;

        // Byte-exact round-trip: serialize the parsed record to a temp file
        // and re-read it.
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
        const QString writtenPath = written.fileName();

        ESMReader check(writtenPath);
        check.open();
        bool found = false;
        while (check.isLeft())
        {
            NAME name = check.readName();
            if (name == 0) break;
            if (name == (NAME)'GRUP') { check.skipGrupHeader(); continue; }
            if (name == (NAME)'PNDT')
            {
                PndRecord roundTrip;
                roundTrip.load(check, true);
                found = true;
                if (rec.editorId != roundTrip.editorId ||
                    rec.flags != roundTrip.flags ||
                    rec.starSystem != roundTrip.starSystem ||
                    std::abs(rec.temperature - roundTrip.temperature) > 0.0001f ||
                    std::abs(rec.density - roundTrip.density) > 0.0001f ||
                    std::abs(rec.phase - roundTrip.phase) > 0.0001f ||
                    rec.resources != roundTrip.resources ||
                    rec.rawSubRecords.size() != roundTrip.rawSubRecords.size() ||
                    rec.mOrder != roundTrip.mOrder)
                {
                    ++failures;
                    qWarning() << "Round-trip mismatch on " << rec.editorId;
                }
                for (int i = 0; i < rec.rawSubRecords.size(); ++i)
                {
                    if (rec.rawSubRecords[i].name != roundTrip.rawSubRecords[i].name ||
                        rec.rawSubRecords[i].data != roundTrip.rawSubRecords[i].data)
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

    qDebug() << "PNDT records round-tripped:" << roundTripped
             << "failures:" << failures;
    QVERIFY(roundTripped >= 20);
    QCOMPARE(failures, 0);
}

QTEST_MAIN(TestPndRecord)
#include "test_pndrecord.moc"
