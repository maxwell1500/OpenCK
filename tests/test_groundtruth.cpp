#include <QtTest>
#include <QFile>
#include <QSet>
#include <QMap>
#include <QStringList>
#include <QDebug>

#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/common.hpp"
#include "../../model/world/ckid.hpp"

// Ground-truth scan of a real Starfield.esm. The record-type names observed
// on disk are cross-checked against CkId::stringToType: every record type the
// master actually emits must resolve to a CkId alias (and the bare/underscore
// spellings must agree). The reverse direction — which known aliases the
// master does not emit — is reported as warnings, not failures: a vanilla
// master can legitimately be missing record types that exist in other games.

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
    QSet<QString> mRecordTypes;
    QMap<QString, int> mTypeCounts;

    void scanFile(ESMReader& reader)
    {
        // Flat walk, same as the dumpesm diagnostic: groups are entered by
        // skipping their 24-byte header, never by recursing, so the file
        // position always refers to the real stream. seekTo(filePos())
        // round-trips would misalign: while a compressed record is open the
        // filePos() reports the in-memory buffer position, not the file.
        while (reader.isLeft())
        {
            NAME name = 0;
            try
            {
                name = reader.readName();
            }
            catch (...)
            {
                break;
            }
            if (name == 0) break;

            if (name == (NAME)'GRUP')
            {
                try
                {
                    reader.skipGrupHeader();
                }
                catch (...)
                {
                    break;
                }
                continue;
            }

            const QString recStr = nameToString(name);
            if (recStr.size() == 4)
            {
                mRecordTypes.insert(recStr);
                mTypeCounts[recStr]++;
            }

            try
            {
                reader.skipRecord();
            }
            catch (...)
            {
                break;
            }
        }
    }

private slots:
    void initTestCase()
    {
        try
        {
            mFilePath = "C:/XboxGames/Starfield/Content/Data/Starfield.esm";
            QVERIFY2(QFile::exists(mFilePath), "Starfield.esm not found");
            ESMReader reader(mFilePath);
            reader.open();
            scanFile(reader);
            QVERIFY2(!mRecordTypes.isEmpty(), "scan observed no record types");
        }
        catch (const std::exception& e)
        {
            qFatal("initTestCase threw: %s", e.what());
        }
    }

    void testRecordTypesResolve();
    void testKnownAliasesCoveredByScan();
};

void TestGroundTruth::testRecordTypesResolve()
{
    qWarning() << "record types observed in Starfield.esm:" << mRecordTypes.size();

    QStringList unresolved;
    for (const QString& t : mRecordTypes)
    {
        if (t == "TES4" || t == "GRUP")
            continue;
        const CkId::Type bare = CkId::stringToType(t);
        const CkId::Type trail = CkId::stringToType(t + QLatin1Char('_'));
        if (bare == CkId::Type_None && trail == CkId::Type_None)
            unresolved << t;
        else if (bare != CkId::Type_None && trail != CkId::Type_None && bare != trail)
            unresolved << (t + " (bare/underscore mapping conflict)");
    }
    QVERIFY2(unresolved.isEmpty(),
        qPrintable(QString("Record types in Starfield.esm without a CkId alias: %1")
            .arg(unresolved.join(", "))));
}

void TestGroundTruth::testKnownAliasesCoveredByScan()
{
    QStringList missing;
    int aliasable = 0;
    for (int t = CkId::Type_Npc_; t < CkId::NumTypes; ++t)
    {
        const QString disk = CkId(static_cast<CkId::Type>(t)).getTypeName();
        if (disk.size() != 4)
            continue;
        ++aliasable;
        if (!mRecordTypes.contains(disk))
            missing << disk;
    }
    qWarning() << "known 4CC record aliases:" << aliasable
               << "observed:" << mRecordTypes.size();
    if (!missing.isEmpty())
        qWarning() << "known record types not observed in Starfield.esm (n="
                   << missing.size() << "):" << missing.join(", ");
}

QTEST_MAIN(TestGroundTruth)
#include "test_groundtruth.moc"
