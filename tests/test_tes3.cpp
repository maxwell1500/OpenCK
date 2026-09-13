#include <QtTest>
#include <QFile>
#include <QFileInfo>
#include <QHash>

#include "esmreader.hpp"
#include "tes4.hpp"
#include "common.hpp"
#include "gameformat.hpp"
#include "logger.hpp"

// Ground-truth tests for the TES3 (Morrowind) reader against the real
// Morrowind.esm: 16-byte record headers, 8-byte subrecord headers, no
// compression, fixed-size strings.
class TestTes3 : public QObject
{
    Q_OBJECT

private:
    QString fixture() const
    {
        return qEnvironmentVariable(
            "OPENCK_TEST_MORROWIND_ESM",
            QStringLiteral("C:/XboxGames/The Elder Scrolls III- Morrowind (PC)/Content/"
                           "Morrowind GOTY English/Data Files/Morrowind.esm"));
    }

private slots:
    void initTestCase();
    void testHeader();
    void testFullWalk();
    void testBuildRecordIndex();
    void testGmstSubrecords();
};

void TestTes3::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
}

void TestTes3::testHeader()
{
    const QString path = fixture();
    if (!QFile::exists(path))
    {
        QSKIP("Morrowind.esm fixture not available (set OPENCK_TEST_MORROWIND_ESM)");
    }

    ESMReader reader(path);
    reader.open();

    QVERIFY2(reader.tes3(), "reader did not detect the TES3 magic");

    const Header& h = reader.getHeader();
    // HEDR: version 1.2 (Bloodmoon/GOTY), type 1 (master), 48295 records.
    QCOMPARE(qAbs(h.version - 1.2f) < 0.001f, true);
    QCOMPARE(h.tes3FileType, quint32(1));
    QCOMPARE(h.numRecords, qint32(48295));
    QCOMPARE(h.author, QString("Bethesda Softworks"));
    QVERIFY2(h.description.contains("Morrowind"),
             qPrintable(h.description));
    QVERIFY2(h.masters.isEmpty(), "base master has no masters");
    QCOMPARE(h.formatVersion, quint32(0));
    // The first record starts right after the 16-byte TES3 header plus the
    // 308-byte HEDR subrecord: 16 + 308 = 324.
    QCOMPARE(reader.filePos(), qint64(324));
}

// Walks the entire file record by record. Every declared size field must be
// consistent or the walk desyncs, stops early (name == 0), or runs off the
// end; the final count must equal HEDR's record count exactly.
void TestTes3::testFullWalk()
{
    const QString path = fixture();
    if (!QFile::exists(path))
    {
        QSKIP("Morrowind.esm fixture not available (set OPENCK_TEST_MORROWIND_ESM)");
    }

    ESMReader reader(path);
    reader.open();

    const Header& h = reader.getHeader();
    quint64 count = 0;
    QHash<NAME, int> types;

    while (reader.isLeft())
    {
        NAME name = reader.readName();
        if (name == 0)
        {
            break;
        }

        if (name == NAME('GRUP'))
        {
            // Group: 16-byte header, then the inner records up to its end.
            reader.skipGrupHeader();
            while (reader.filePos() < reader.grupEnd()
                   && reader.grupEnd() - reader.filePos() >= 16)
            {
                NAME inner = reader.readName();
                if (inner == 0)
                {
                    break;
                }
                reader.readHeader();
                reader.skipRemainingRecord();
                types[inner]++;
                count++;
            }
            reader.skipToGrupEnd();
            continue;
        }

        reader.readHeader();
        reader.skipRemainingRecord();
        types[name]++;
        count++;
    }

    qInfo() << "TES3 walk: counted" << count << "records, types:";
    for (auto it = types.constBegin(); it != types.constEnd(); ++it)
    {
        char buf[5] = { char((it.key() >> 24) & 0xFF), char((it.key() >> 16) & 0xFF),
                        char((it.key() >> 8) & 0xFF), char(it.key() & 0xFF), 0 };
        qInfo() << QString("  %1 %2").arg(buf).arg(it.value());
    }

    QCOMPARE(count, quint64(h.numRecords));
    // Core Morrowind record types must be present.
    QVERIFY(types.contains(NAME('GMST')));
    QVERIFY(types.contains(NAME('NPC_')));
    QVERIFY(types.contains(NAME('CELL')));
    QVERIFY(types.contains(NAME('SCPT')));
    QVERIFY(types.contains(NAME('DIAL')));
    QVERIFY(types.contains(NAME('WEAP')));
    QVERIFY(types.contains(NAME('ARMO')));
    QVERIFY(types.contains(NAME('BOOK')));
    QVERIFY(types.contains(NAME('SPEL')));
    QVERIFY(types.contains(NAME('MGEF')));
}

void TestTes3::testBuildRecordIndex()
{
    const QString path = fixture();
    if (!QFile::exists(path))
    {
        QSKIP("Morrowind.esm fixture not available (set OPENCK_TEST_MORROWIND_ESM)");
    }

    ESMReader reader(path);
    reader.open();

    QVector<RecordIndexEntry> index;
    reader.buildRecordIndex(index);

    QCOMPARE(index.size(), int(reader.getHeader().numRecords));
    QVERIFY2(index.size() > 0, "index empty");
    // The first record sits 324 bytes in (see testHeader); Morrowind stores
    // its game settings first.
    QCOMPARE(index.first().offset, qint64(324));
    QCOMPARE(index.first().type, NAME('GMST'));
}

// Fully parses the subrecords of the first GMST record: NAME (variable
// length string) followed by STRV (4-byte float for numeric globals, or a
// string for string globals — the size field disambiguates). After
// draining, the record must be exactly consumed.
void TestTes3::testGmstSubrecords()
{
    const QString path = fixture();
    if (!QFile::exists(path))
    {
        QSKIP("Morrowind.esm fixture not available (set OPENCK_TEST_MORROWIND_ESM)");
    }

    ESMReader reader(path);
    reader.open();

    NAME name = 0;
    while (reader.isLeft())
    {
        name = reader.readName();
        if (name == 0)
        {
            break;
        }
        if (name == NAME('GMST'))
        {
            break;
        }
        reader.readHeader();
        reader.skipRemainingRecord();
    }

    QVERIFY2(name == NAME('GMST'), "no GMST record found");

    reader.readHeader();
    QVERIFY(reader.isRecLeft());

    QString key = reader.readSubZString(NAME('NAME'));
    QVERIFY2(!key.isEmpty(), "empty GMST NAME");
    qInfo() << "First GMST key:" << key;
    QCOMPARE(key, QString("sMonthMorningstar"));

    QVERIFY(reader.isNextName(NAME('STRV')));
    reader.readNSubHeader();
    QByteArray strv;
    reader.readRawSubData(strv);
    QVERIFY2(!strv.isEmpty(), "empty GMST STRV");

    // The GMST body is exactly NAME + STRV; nothing may be left over.
    QCOMPARE(reader.recLeft(), qint64(0));
}

QTEST_MAIN(TestTes3)
#include "test_tes3.moc"
