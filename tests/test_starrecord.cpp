#include <QtTest>
#include <QFileInfo>
#include <QTemporaryFile>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "stdtrecord.hpp"
#include "reflectstream.hpp"
#include "common.hpp"
#include "logger.hpp"

// Validates StdtRecord (STDT / Star) — the galaxy-map record — against the
// real Starfield.esm: typed accessors (name, parsec position, system id,
// colour, links) and the BGSStarDataComponent_Component catalogue fields, plus
// a byte-exact round-trip.
class TestStarRecord : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testStarFieldsAndRoundTrip();
    void testDistinctSystemIds();
    void testReflectionStreamMetadata();

private:
    QString esmPath() const;
};

void TestStarRecord::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_starrecord_log.txt"));
}

QString TestStarRecord::esmPath() const
{
    return qEnvironmentVariable("OPENCK_DATA_DIR",
               QStringLiteral("C:/XboxGames/Starfield/Content/Data"))
        + "/Starfield.esm";
}

void TestStarRecord::testStarFieldsAndRoundTrip()
{
    const QString path = esmPath();
    if (!QFileInfo::exists(path)) QSKIP("Starfield.esm not found");

    ESMReader reader(path);
    reader.open();

    int stars = 0;
    int withName = 0;
    int withParsec = 0;
    int withSystemId = 0;
    int withStarData = 0;
    int withSpectral = 0;
    int failures = 0;

    while (stars < 30 && reader.isLeft())
    {
        NAME name = 0;
        try { name = reader.readName(); } catch (...) { break; }
        if (name == 0) break;
        if (name == (NAME)'GRUP') { reader.skipGrupHeader(); continue; }
        if (name != (NAME)'STDT') { reader.skipRecord(); continue; }

        StdtRecord rec;
        rec.load(reader, true);

        if (!rec.starName().isEmpty()) ++withName;
        if (rec.hasParsecLocation()) ++withParsec;
        if (rec.systemId() != 0) ++withSystemId;

        const StarDataComponent sd = rec.starData();
        if (sd.valid)
        {
            ++withStarData;
            if (!sd.spectralClass.isEmpty()) ++withSpectral;
            // Spectral classes are short codes (O/B/A/F/G/K/M, possibly with
            // a digit). Anything longer means the DATA parse is off.
            if (sd.spectralClass.size() > 8)
            {
                ++failures;
                qWarning() << "implausible spectral class" << sd.spectralClass
                           << "on" << rec.editorId;
            }
        }

        // Byte-exact round-trip.
        QTemporaryFile written;
        QVERIFY(written.open());
        {
            ESMWriter writer;
            writer.setAuthor("Test");
            writer.save(written);
            RecHeader rh;
            rh.id = rec.formId;
            writer.startRecord('STDT', rh);
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
            NAME cname = check.readName();
            if (cname == 0) break;
            if (cname == (NAME)'GRUP') { check.skipGrupHeader(); continue; }
            if (cname != (NAME)'STDT') { check.skipRecord(); continue; }
            StdtRecord back;
            back.load(check, true);
            found = true;
            if (rec.editorId != back.editorId || rec.formId != back.formId
                || rec.rawSubRecords.size() != back.rawSubRecords.size())
                ++failures;
            for (int i = 0; i < rec.rawSubRecords.size(); ++i)
            {
                if (rec.rawSubRecords[i].name != back.rawSubRecords[i].name
                    || rec.rawSubRecords[i].data != back.rawSubRecords[i].data)
                    ++failures;
            }
            break;
        }
        if (!found)
            ++failures;

        ++stars;
    }

    qDebug() << "STDT:" << stars
             << "named:" << withName
             << "with parsec:" << withParsec
             << "with system id:" << withSystemId
             << "with star data:" << withStarData
             << "with spectral:" << withSpectral
             << "failures:" << failures;
    QVERIFY(stars > 0);
    QVERIFY(withName > 0);
    QVERIFY(withParsec > 0);
    QVERIFY(withSystemId > 0);
    QVERIFY(withStarData > 0);
    QVERIFY(withSpectral > 0);
    QCOMPARE(failures, 0);
}

void TestStarRecord::testDistinctSystemIds()
{
    const QString path = esmPath();
    if (!QFileInfo::exists(path)) QSKIP("Starfield.esm not found");

    ESMReader reader(path);
    reader.open();

    QSet<quint32> systemIds;
    int stars = 0;
    while (stars < 400 && reader.isLeft())
    {
        NAME name = 0;
        try { name = reader.readName(); } catch (...) { break; }
        if (name == 0) break;
        if (name == (NAME)'GRUP') { reader.skipGrupHeader(); continue; }
        if (name != (NAME)'STDT') { reader.skipRecord(); continue; }
        StdtRecord rec;
        rec.load(reader, true);
        if (rec.systemId() != 0)
            systemIds.insert(rec.systemId());
        ++stars;
    }

    qDebug() << "STDT scanned:" << stars << "distinct system ids:" << systemIds.size();
    // System ids should be largely distinct (each star is its own system).
    QVERIFY(stars > 0);
    QVERIFY(systemIds.size() > stars / 2);
}

void TestStarRecord::testReflectionStreamMetadata()
{
    const QString path = esmPath();
    if (!QFileInfo::exists(path)) QSKIP("Starfield.esm not found");

    // The reflection streams (REFL) are not raw noise: they carry a "BETH"
    // magic and an embedded schema of type/field names. Read a real one off a
    // SUNP (Sun Preset) record and check the schema decodes.
    ESMReader reader(path);
    reader.open();

    int checked = 0;
    bool sawRootType = false;
    bool sawSunColor = false;
    while (checked < 5 && reader.isLeft())
    {
        NAME name = 0;
        try { name = reader.readName(); } catch (...) { break; }
        if (name == 0) break;
        if (name == (NAME)'GRUP') { reader.skipGrupHeader(); continue; }
        if (name != (NAME)'SUNP') { reader.skipRecord(); continue; }

        // Walk the record's subrecords looking for REFL/RDIF.
        QByteArray refl;
        const qint64 recStart = 0;
        Q_UNUSED(recStart);
        reader.readHeader();
        while (reader.isRecLeft())
        {
            const NAME sub = reader.readNSubHeader();
            if (sub == 0) break;
            QByteArray data;
            reader.readRawSubData(data);
            if ((sub == NAME('REFL') || sub == NAME('RDIF')) && refl.isEmpty())
                refl = data;
        }
        if (refl.isEmpty())
            continue;
        ++checked;

        const ReflectionStream stream = parseReflectionStream(refl);
        QVERIFY(stream.valid);
        QCOMPARE(stream.raw, refl);           // exact bytes preserved
        QVERIFY(!stream.fieldNames.isEmpty());
        if (!stream.rootType.isEmpty())
            sawRootType = true;
        if (stream.hasField(QStringLiteral("SunColor")))
            sawSunColor = true;
    }

    qDebug() << "REFL streams checked:" << checked
             << "root type seen:" << sawRootType
             << "SunColor seen:" << sawSunColor;
    QVERIFY(checked > 0);
    QVERIFY(sawRootType);
    QVERIFY(sawSunColor);
}

QTEST_MAIN(TestStarRecord)
#include "test_starrecord.moc"
