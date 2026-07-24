#include <QtTest>
#include <QFile>
#include <QDataStream>
#include <QByteArray>
#include <QMap>
#include <QSet>
#include <QDebug>

#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/tes4.hpp"
#include "../../libs/files/esm/tes4codes.hpp"
#include "../../libs/files/esm/common.hpp"
#include "../../libs/files/log/logger.hpp"

static QString nameToString(NAME n)
{
    if (n == 0) return "NULL";
    return QString(QChar((n >> 24) & 0xFF)) +
           QString(QChar((n >> 16) & 0xFF)) +
           QString(QChar((n >> 8) & 0xFF)) +
           QString(QChar(n & 0xFF));
}

class TestGroundTruth : public QObject
{
    Q_OBJECT

    QString mFilePath;
    QMap<QString, QMap<QString, int>> mRawCodeCount;
    QMap<QString, QMap<QString, QPair<QString, int>>> mTranslationCount;

    void processRecord(ESMReader& reader, NAME recName, qint64 startPos, int depth = 0)
    {
        QString recStr = nameToString(recName);
        QString indent = QString("  ").repeated(depth);

        while (reader.isRecLeft())
        {
            // Peek at the next 4 bytes to see if it's a subrecord or a GRUP
            if (reader.recLeft() < 6) break;

            // Save position before readRawNSubHeader
            NAME rawName = reader.readRawNSubHeader();
            if (rawName == 0) break;

            int subDataSize = static_cast<int>(reader.subLeft());

            NAME translatedName = Tes4Codes::fromTes4(rawName);
            QString rawStr = nameToString(rawName);
            QString xlatStr = nameToString(translatedName);

            mRawCodeCount[recStr][rawStr]++;
            if (rawStr != xlatStr) {
                mTranslationCount[recStr][rawStr].first = xlatStr;
                mTranslationCount[recStr][rawStr].second++;
            }

            // Skip subrecord data
            reader.skip(subDataSize);
        }
    }

    void traverseFile(ESMReader& reader)
    {
        while (reader.isLeft())
        {
            qint64 pos = reader.filePos();
            NAME name = reader.readName();
            if (name == 0) break;

            if (name == (NAME)'GRUP')
            {
                reader.skipGrupHeader();
                qint64 grupEnd = reader.grupEnd();

                // For GRUP records, iterate their children
                while (reader.filePos() < grupEnd && reader.isLeft())
                {
                    NAME childName = reader.readName();
                    if (childName == 0) break;

                    // Seek back to re-read the child name with readHeader
                    reader.seekTo(reader.filePos() - 4);

                    if (childName == (NAME)'GRUP')
                    {
                        // Nested GRUP
                        reader.readName();
                        reader.skipGrupHeader();
                        qint64 nestedGrupEnd = reader.grupEnd();
                        reader.seekTo(nestedGrupEnd);
                    }
                    else
                    {
                        // Regular record — read header, process subrecords, skip
                        RecHeader header = reader.readHeader();
                        qint64 recEnd = reader.filePos() + reader.recLeft();

                        processRecord(reader, childName, reader.filePos(), 1);

                        // Skip any remaining bytes in this record
                        if (reader.recLeft() > 0)
                            reader.skipRemainingRecord();
                        reader.seekTo(recEnd);
                    }
                }

                reader.seekTo(grupEnd);
            }
            else
            {
                // Top-level record (unusual for Starfield but handle it)
                reader.seekTo(pos);
                reader.readName(); // re-read
                RecHeader header = reader.readHeader();
                processRecord(reader, name, reader.filePos(), 0);
                if (reader.recLeft() > 0)
                    reader.skipRemainingRecord();
            }
        }
    }

private slots:
    void initTestCase()
    {
        mFilePath = "C:/XboxGames/Starfield/Content/Data/Starfield.esm";
        QVERIFY2(QFile::exists(mFilePath), "Starfield.esm not found");
    }

    void testDumpSubrecords()
    {
        ESMReader reader(mFilePath);
        reader.open();

        traverseFile(reader);

        qDebug() << "\n=== GROUND TRUTH: RAW ON-DISK SUBRECORD CODES ===";
        qDebug() << "Format: RecordType -> (raw_code, count)";
        qDebug() << "";

        for (auto it = mRawCodeCount.begin(); it != mRawCodeCount.end(); ++it)
        {
            QString recName = it.key();
            auto& codes = it.value();

            // Sort by code
            QStringList sortedCodes = codes.keys();
            sortedCodes.sort();

            qDebug().noquote() << QString("[%1]").arg(recName);
            for (const QString& code : sortedCodes)
            {
                int count = codes[code];
                QString note;
                if (mTranslationCount.contains(recName) && mTranslationCount[recName].contains(code))
                {
                    QString xlat = mTranslationCount[recName][code].first;
                    note = QString("  -> translated to %1 by Tes4Codes::fromTes4()").arg(xlat);
                }
                qDebug().noquote() << QString("    %1 (0x%2) x%3%4")
                    .arg(code)
                    .arg(QString::number((code[0].unicode() << 24) | (code[1].unicode() << 16) | (code[2].unicode() << 8) | code[3].unicode(), 16))
                    .arg(count)
                    .arg(note);
            }
            qDebug() << "";
        }

        qDebug() << "=== TRANSLATION SUMMARY ===";
        qDebug() << "Raw code -> Translated code (occurs in record types):";
        QMap<QString, QSet<QString>> xlatToRec;
        for (auto it = mTranslationCount.begin(); it != mTranslationCount.end(); ++it)
        {
            QString recName = it.key();
            auto& codes = it.value();
            for (auto cit = codes.begin(); cit != codes.end(); ++cit)
            {
                QString raw = cit.key();
                QString xlat = cit.value().first;
                xlatToRec[QString("%1 -> %2").arg(raw).arg(xlat)].insert(recName);
            }
        }
        for (auto it = xlatToRec.begin(); it != xlatToRec.end(); ++it)
        {
            QStringList recs = it.value().values();
            recs.sort();
            qDebug().noquote() << QString("  %1  [%2]")
                .arg(it.key(), -30)
                .arg(recs.join(", "));
        }
    }
};

QTEST_MAIN(TestGroundTruth)
#include "test_groundtruth.moc"
