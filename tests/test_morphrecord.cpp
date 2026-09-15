#include <QtTest>
#include <QBuffer>
#include <QFileInfo>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "mrhprecord.hpp"
#include "common.hpp"
#include "logger.hpp"

// Validates the MrhpRecord binary codec against the real Starfield.esm.
class TestMorphRecord : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testSurveyRealRecords();
};

void TestMorphRecord::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_morphrecord_log.txt"));
}

static bool parseNext(ESMReader& reader, MrhpRecord& rec, int* recordsScanned)
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
        if (name == (NAME)'MRPH')
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

void TestMorphRecord::testSurveyRealRecords()
{
    const QString filePath = qEnvironmentVariable("OPENCK_DATA_DIR", QStringLiteral("C:/XboxGames/Starfield/Content/Data")) + "/Starfield.esm";
    if (!QFileInfo::exists(filePath)) QSKIP("Starfield.esm not found");

    ESMReader reader(filePath);
    reader.open();

    int roundTripped = 0;
    int scanned = 0;
    int failures = 0;
    int withTcmp = 0;
    int withTmpp = 0;
    while (scanned < 10000000)
    {
        MrhpRecord rec;
        if (!parseNext(reader, rec, &scanned))
            break;
        if (roundTripped >= 20) break;

        if (!rec.morphPath.isEmpty()) ++withTcmp;
        if (!rec.templatePath.isEmpty()) ++withTmpp;

        // Byte-exact round-trip
        QTemporaryFile written;
        QVERIFY(written.open());
        {
            ESMWriter writer;
            writer.setAuthor("Test");
            writer.save(written);
            RecHeader rh;
            rh.id = rec.formId;
            writer.startRecord('MRPH', rh);
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
            if (name == (NAME)'MRPH')
            {
                MrhpRecord roundTrip;
                roundTrip.load(check, true);
                found = true;
                if (rec.editorId != roundTrip.editorId ||
                    rec.morphPath != roundTrip.morphPath ||
                    rec.mobcFlags != roundTrip.mobcFlags ||
                    rec.templatePath != roundTrip.templatePath ||
                    rec.rawSubRecords.size() != roundTrip.rawSubRecords.size())
                {
                    ++failures;
                    qWarning() << "Round-trip mismatch on" << rec.editorId;
                }
                for (int i = 0; i < rec.rawSubRecords.size(); ++i)
                {
                    if (rec.rawSubRecords[i].name != roundTrip.rawSubRecords[i].name ||
                        rec.rawSubRecords[i].data != roundTrip.rawSubRecords[i].data)
                    {
                        ++failures;
                        qWarning() << "Raw subrecord mismatch on" << rec.editorId
                                   << "at index" << i;
                    }
                }
                ++roundTripped;
                break;
            }
            check.skipRecord();
        }
    }

    qDebug() << "MRPH records round-tripped:" << roundTripped
             << "with TCMP:" << withTcmp
             << "with TMPP:" << withTmpp
             << "failures:" << failures;
    QVERIFY(roundTripped >= 20);
    QCOMPARE(failures, 0);
    QVERIFY(withTcmp > 0);
}

QTEST_MAIN(TestMorphRecord)
#include "test_morphrecord.moc"
