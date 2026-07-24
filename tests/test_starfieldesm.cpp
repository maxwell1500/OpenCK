#include <QtTest>
#include <QFile>
#include <QDataStream>
#include <QByteArray>

#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/tes4.hpp"
#include "../../libs/files/esm/common.hpp"
#include "../../libs/files/log/logger.hpp"

class TestStarfieldESM : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testLoadStarfieldHeader();
    void testIterateFirstRecords();
};

void TestStarfieldESM::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init("C:/Users/max/Projects/OpenCK/test_starfield_log.txt");
}

void TestStarfieldESM::testLoadStarfieldHeader()
{
    QString filePath = "C:/XboxGames/Starfield/Content/Data/Starfield.esm";
    QVERIFY(QFile::exists(filePath));
    qint64 fileSize = QFileInfo(filePath).size();
    qDebug() << "Starfield.esm size:" << fileSize << "bytes";

    ESMReader reader(filePath);
    try {
        reader.open();
    } catch (const std::exception& e) {
        qWarning() << "open() threw:" << e.what();
        QFAIL("Failed to open Starfield.esm");
    }

    qDebug() << "Header version:" << reader.getHeader().version;
    qDebug() << "Header numRecords:" << reader.getHeader().numRecords;
    qDebug() << "Header author:" << reader.getHeader().author;
    qDebug() << "Header description:" << reader.getHeader().description.left(200);
    qDebug() << "Header masters count:" << reader.getHeader().masters.size();

    QVERIFY(reader.getHeader().version > 0.0f);
    QVERIFY(reader.getHeader().numRecords > 0);
}

void TestStarfieldESM::testIterateFirstRecords()
{
    QString filePath = "C:/XboxGames/Starfield/Content/Data/Starfield.esm";
    ESMReader reader(filePath);
    try {
        reader.open();
    } catch (const std::exception& e) {
        QFAIL("Failed to open");
    }

    int recordsRead = 0;
    int errors = 0;
    int grpCount = 0, kywdCount = 0, otherCount = 0;
    int compressedCount = 0;
    while (recordsRead < 200 && reader.isLeft())
    {
        qint64 pos = reader.filePos();
        NAME name = 0;
        try {
            name = reader.readName();
        } catch (const std::exception& e) {
            qWarning() << "  readName threw:" << e.what();
            errors++;
            break;
        } catch (...) {
            errors++;
            break;
        }

        if (name == 0)
        {
            qDebug() << "Read 0 at pos" << QString::number(pos, 16) << "- end of file";
            break;
        }

        if (name == (NAME)'GRUP') {
            grpCount++;
        } else if (name == (NAME)'KYWD') {
            kywdCount++;
        } else {
            otherCount++;
        }

        // Don't log every record, just sample
        if (recordsRead < 5 || recordsRead % 20 == 0) {
            qDebug() << "Record" << recordsRead << "at 0x" << QString::number(pos, 16)
                     << "name=" << QChar((name >> 24) & 0xFF)
                              << QChar((name >> 16) & 0xFF)
                              << QChar((name >> 8) & 0xFF)
                              << QChar(name & 0xFF);
        }

        try {
            RecHeader header = reader.readHeader();
            bool isCompressed = (header.flags.val & 0x00040000) != 0;
            if (isCompressed) {
                compressedCount++;
                if (compressedCount <= 5) {
                    qDebug() << "  COMPRESSED at 0x" << QString::number(pos, 16)
                             << " recSize=" << header.size;
                }
            }
            // Skip the record's data
            reader.skipRemainingRecord();
        } catch (const std::exception& e) {
            qWarning() << "  readHeader/skip failed:" << e.what();
            errors++;
            break;
        }

        recordsRead++;
    }

    qDebug() << "Records read:" << recordsRead
             << "(GRUP:" << grpCount
             << " KYWD:" << kywdCount
             << " other:" << otherCount
             << " compressed:" << compressedCount << ")"
             << "Errors:" << errors;
    QVERIFY(recordsRead >= 50);
    QVERIFY(errors == 0);
}

QTEST_MAIN(TestStarfieldESM)
#include "test_starfieldesm.moc"
