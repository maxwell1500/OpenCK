#include <QtTest>
#include <QFile>
#include <QFileInfo>

#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/tes4.hpp"
#include "../../libs/files/esm/common.hpp"
#include "../../libs/files/esm/gameformat.hpp"
#include "../../libs/files/log/logger.hpp"

using namespace GameFormat;

class TestMultiGameESM : public QObject
{
    Q_OBJECT

private:
    struct Result
    {
        QString path;
        bool opened = false;
        quint32 numRecords = 0;
        float version = 0.0f;
        QStringList masters;
        quint32 recordsRead = 0;
        quint32 errors = 0;
    };

    Result loadSmoke(const QString& path, int maxRecords)
    {
        Result r;
        r.path = path;
        if (!QFile::exists(path))
        {
            return r;
        }

        ESMReader reader(path);
        try
        {
            reader.open();
            r.opened = true;
        }
        catch (const std::exception& e)
        {
            qWarning() << "open() failed:" << e.what();
            return r;
        }

        const Header& h = reader.getHeader();
        r.numRecords = h.numRecords;
        r.version = h.version;
        for (const MasterData& m : h.masters)
        {
            r.masters.append(m.name);
        }

        while (r.recordsRead < static_cast<quint32>(maxRecords) && reader.isLeft())
        {
            NAME name = 0;
            try
            {
                name = reader.readName();
            }
            catch (const std::exception& e)
            {
                qWarning() << "readName failed:" << e.what();
                r.errors++;
                break;
            }
            catch (...)
            {
                r.errors++;
                break;
            }

            if (name == 0)
            {
                break;
            }

            try
            {
                reader.readHeader();
                reader.skipRemainingRecord();
            }
            catch (const std::exception& e)
            {
                qWarning() << "readHeader/skip failed:" << e.what();
                r.errors++;
                break;
            }
            catch (...)
            {
                r.errors++;
                break;
            }

            r.recordsRead++;
        }

        return r;
    }

private slots:
    void initTestCase();
    void testStarfield();
    void testFallout4();
    void testMorrowind();
};

void TestMultiGameESM::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
}

void TestMultiGameESM::testStarfield()
{
    QString path = qEnvironmentVariable(
        "OPENCK_TEST_STARFIELD_ESM",
        QStringLiteral("C:/XboxGames/Starfield/Content/Data/Starfield.esm"));
    if (!QFile::exists(path))
    {
        QSKIP("Starfield.esm fixture not available (set OPENCK_TEST_STARFIELD_ESM)");
    }
    Result r = loadSmoke(path, 400);
    qDebug() << "Starfield:" << r.path
             << "opened=" << r.opened
             << "numRecords=" << r.numRecords
             << "version=" << r.version
             << "recordsRead=" << r.recordsRead
             << "errors=" << r.errors
             << "masters=" << r.masters;
    QCOMPARE(detectGame(QFileInfo(path).fileName(), r.masters, quint16(0)),
             Game::Starfield);
    QVERIFY2(r.opened, qPrintable(r.path));
    QVERIFY2(r.numRecords > 0, "numRecords");
    QVERIFY2(r.recordsRead >= 100, "recordsRead");
    QVERIFY2(r.errors == 0, "errors");
}

void TestMultiGameESM::testFallout4()
{
    QString path = qEnvironmentVariable(
        "OPENCK_TEST_FO4_ESM",
        QStringLiteral("H:/New folder/Fallout 4/Content/Data/Fallout4.esm"));
    if (!QFile::exists(path))
    {
        QSKIP("Fallout4.esm fixture not available (set OPENCK_TEST_FO4_ESM)");
    }
    Result r = loadSmoke(path, 400);
    qDebug() << "Fallout4:" << r.path
             << "opened=" << r.opened
             << "numRecords=" << r.numRecords
             << "version=" << r.version
             << "recordsRead=" << r.recordsRead
             << "errors=" << r.errors
             << "masters=" << r.masters;
    QCOMPARE(detectGame(QFileInfo(path).fileName(), r.masters, quint16(0)),
             Game::Fallout4);
    QVERIFY2(r.opened, qPrintable(r.path));
    QVERIFY2(r.numRecords > 0, "numRecords");
    QVERIFY2(r.recordsRead >= 100, "recordsRead");
    QVERIFY2(r.errors == 0, "errors");
}

void TestMultiGameESM::testMorrowind()
{
    QString path = qEnvironmentVariable(
        "OPENCK_TEST_MORROWIND_ESM",
        QStringLiteral("C:/XboxGames/The Elder Scrolls III- Morrowind (PC)/Content/"
                       "Morrowind GOTY English/Data Files/Morrowind.esm"));
    if (!QFile::exists(path))
    {
        QSKIP("Morrowind.esm fixture not available (set OPENCK_TEST_MORROWIND_ESM)");
    }
    Result r = loadSmoke(path, 400);
    qDebug() << "Morrowind:" << r.path
             << "opened=" << r.opened
             << "numRecords=" << r.numRecords
             << "version=" << r.version
             << "recordsRead=" << r.recordsRead
             << "errors=" << r.errors
             << "masters=" << r.masters;
    QCOMPARE(detectGame(QFileInfo(path).fileName(), r.masters, quint16(0)),
             Game::Morrowind);
    QVERIFY2(r.opened, qPrintable(r.path));
    QVERIFY2(r.numRecords > 0, "numRecords");
    QVERIFY2(r.recordsRead >= 100, "recordsRead");
    QVERIFY2(r.errors == 0, "errors");
}

QTEST_MAIN(TestMultiGameESM)
#include "test_multigame.moc"
