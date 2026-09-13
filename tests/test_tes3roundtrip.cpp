#include <QtTest>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "../../src/model/world/data.hpp"
#include "../../src/model/world/ckid.hpp"
#include "../../src/model/doc/messages.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/tes4.hpp"
#include "../../libs/files/esm/gameformat.hpp"
#include "../../libs/files/filepaths.hpp"
#include "../../libs/files/log/logger.hpp"

using namespace GameFormat;

class TestTes3RoundTrip : public QObject
{
    Q_OBJECT

private:
    QString fixturePath() const
    {
        return qEnvironmentVariable(
            "OPENCK_TEST_MORROWIND_ESM",
            QStringLiteral("C:/XboxGames/The Elder Scrolls III- Morrowind (PC)/Content/"
                           "Morrowind GOTY English/Data Files/Morrowind.esm"));
    }

    static QByteArray slurp(const QString& path)
    {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return QByteArray();
        return f.readAll();
    }

private slots:
    void initTestCase();
    void testLoadAndSaveByteIdentical();
    void testPerTypeCounts();
};

void TestTes3RoundTrip::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Error);
    OpenCK::Logging::Logger::instance().init(QStringLiteral(
        "C:/Users/max/AppData/Local/Temp/opencode/test_tes3roundtrip_log.txt"));
}

void TestTes3RoundTrip::testLoadAndSaveByteIdentical()
{
    const QString path = fixturePath();
    if (!QFile::exists(path))
        QSKIP("Morrowind.esm fixture not available (set OPENCK_TEST_MORROWIND_ESM)");

    const QByteArray original = slurp(path);
    QVERIFY(!original.isEmpty());

    FilePaths paths;
    paths.dataDir.setPath(QFileInfo(path).absolutePath());
    const QString fileName = QFileInfo(path).fileName();
    Data data(QStringList{ fileName }, paths);
    QVERIFY2(data.preload(fileName, false) > 0, "preload failed");
    QCOMPARE(data.currentGame(), Game::Morrowind);

    Messages messages(Message::Info);
    int guard = 0;
    while (!data.continueLoading(messages))
    {
        if (++guard > 5000000)
        {
            QFAIL("continueLoading did not converge");
        }
    }

    QCOMPARE(data.pluginOrder().size(), 48295);

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString savedPath = tmp.path() + "/roundtrip.esm";

    {
        QFile out(savedPath);
        QVERIFY(out.open(QIODevice::WriteOnly));

        ESMWriter writer;
        const Header header = data.getReaderHeader();
        writer.setTes3(true);
        writer.setFileFlags(header.flags.val);
        writer.setVersion(header.version);
        writer.setAuthor(header.author);
        writer.setDescription(header.description);
        writer.setNumRecords(header.numRecords);
        writer.setTes3FileType(header.tes3FileType);
        writer.setFormatVersion(header.formatVersion);
        writer.setTes3Gmdt(header.tes3Gmdt);
        writer.setTes3Scrd(header.tes3Scrd);
        writer.setTes3Scrs(header.tes3Scrs);
        writer.save(out);
        data.saveTes3Records(writer);
        writer.close();
    }

    const QByteArray saved = slurp(savedPath);
    QVERIFY(!saved.isEmpty());

    if (saved != original)
    {
        qint64 diffAt = -1;
        const qint64 n = qMin(saved.size(), original.size());
        for (qint64 i = 0; i < n; ++i)
        {
            if (saved.at(i) != original.at(i))
            {
                diffAt = i;
                break;
            }
        }
        if (diffAt < 0)
            diffAt = n;
        QFAIL(qPrintable(QString("round-trip differs at offset %1 (saved %2 bytes, original %3 bytes)")
            .arg(diffAt).arg(saved.size()).arg(original.size())));
    }
    QCOMPARE(saved, original);
}

void TestTes3RoundTrip::testPerTypeCounts()
{
    const QString path = fixturePath();
    if (!QFile::exists(path))
        QSKIP("Morrowind.esm fixture not available (set OPENCK_TEST_MORROWIND_ESM)");

    FilePaths paths;
    paths.dataDir.setPath(QFileInfo(path).absolutePath());
    const QString fileName = QFileInfo(path).fileName();
    Data data(QStringList{ fileName }, paths);
    QVERIFY(data.preload(fileName, false) > 0);

    Messages messages(Message::Info);
    int guard = 0;
    while (!data.continueLoading(messages))
    {
        if (++guard > 5000000)
            QFAIL("continueLoading did not converge");
    }

    struct Expect { NAME code; CkId::Type type; int count; };
    const Expect expect[] = {
        { NAME('GMST'), CkId::Type_Gmst, 1449 },
        { NAME('NPC_'), CkId::Type_Npc_, 2675 },
        { NAME('STAT'), CkId::Type_Stat_, 2788 },
        { NAME('DIAL'), CkId::Type_Dial_, 2358 },
        { NAME('CELL'), CkId::Type_Cel_, 2538 },
        { NAME('LAND'), CkId::Type_Land_, 1390 },
        { NAME('PGRD'), CkId::Type_Pgrd_, 1194 },
        { NAME('BODY'), CkId::Type_Body_, 1125 },
        { NAME('SPEL'), CkId::Type_Spel_, 990 },
        { NAME('SNDG'), CkId::Type_Sndg_, 168 },
        { NAME('SKIL'), CkId::Type_Skil_, 27 },
        { NAME('LEVC'), CkId::Type_Levc_, 116 },
        { NAME('LEVI'), CkId::Type_Levi_, 227 },
        { NAME('LOCK'), CkId::Type_Lock_, 6 },
        { NAME('PROB'), CkId::Type_Prob_, 6 },
        { NAME('REPA'), CkId::Type_Repa_, 6 },
    };

    int total = 0;
    for (const auto& tc : data.allCollectionsWithTypes())
    {
        auto* coll = dynamic_cast<IdCollection<Tes3Record>*>(tc.collection);
        QVERIFY(coll != nullptr);
        total += coll->size();
    }
    QCOMPARE(total, 48295);

    for (const Expect& e : expect)
    {
        auto* coll = dynamic_cast<IdCollection<Tes3Record>*>(
            data.getCollectionByType(e.type));
        QVERIFY2(coll != nullptr, qPrintable(QString("no collection for %1").arg(e.count)));
        QCOMPARE(coll->size(), e.count);
        QVERIFY(coll->size() > 0 && coll->getFormId(0) != 0);
    }
}

QTEST_MAIN(TestTes3RoundTrip)
#include "test_tes3roundtrip.moc"
