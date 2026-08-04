#include <QTest>
#include <QTemporaryFile>
#include <QFile>

#include "model/tools/formidcompactor.hpp"
#include "model/world/data.hpp"
#include "model/world/collection.hpp"
#include "model/world/irecordcollection.hpp"
#include "libs/files/esm/relarecord.hpp"
#include "libs/files/esm/shourecord.hpp"
#include "libs/files/esm/ecznrecord.hpp"
#include "libs/files/esm/ipdsrecord.hpp"
#include "libs/files/esm/statrecord.hpp"
#include "libs/files/filepaths.hpp"
#include "logger.hpp"

// Validates ESL support:
//  - FormIdCompactor remaps a plugin's own (modified) records into the
//    ESL 0x000-0xFFF range, preserving the plugin/master high bits, and
//    rewrites the typed FormID reference fields of records that expose them.
//  - ESMWriter preserves the LightMaster (0x200) file flag on save so a
//    plugin round-trips as an ESL.
class TestEsl : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testCompactorRemapsFormIds();
    void testCompactorRewritesTypedReferences();
    void testCompactorTooManyRecords();
    void testLightMasterFlagRoundTrip();
};

void TestEsl::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral(
        "C:/Users/max/AppData/Local/Temp/opencode/test_esl_log.txt"));
}

void TestEsl::testCompactorRemapsFormIds()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& statCol = data.getStatCollection();

    // Owned records with large local IDs (plugin index 0x10).
    StatRecord a; a.editorId = "StatA"; a.formId = 0x00100050;
    StatRecord b; b.editorId = "StatB"; b.formId = 0x00100080;
    StatRecord c; c.editorId = "StatC"; c.formId = 0x00100030;
    statCol.add(a);
    statCol.add(b);
    statCol.add(c);

    FormIdCompactor compactor(data);
    QCOMPARE(compactor.compact(), 3);
    QCOMPARE(compactor.ownedRecordCount(), 3);
    QCOMPARE(compactor.remappedCount(), 3);

    // Local IDs must be 0x000-0xFFF and unique; high bits preserved.
    QSet<quint32> locals;
    for (int i = 0; i < statCol.count(); ++i)
    {
        const quint32 id = statCol.getFormId(i);
        QVERIFY((id & 0x00000FFF) <= 0x00000FFF);
        QCOMPARE(id & 0xFFFF0000u, 0x00100000u);
        locals.insert(id & 0x00000FFF);
    }
    QCOMPARE(locals.size(), 3);
}

void TestEsl::testCompactorRewritesTypedReferences()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& relaCol = data.getRelaCollection();

    // The RELA record's own ID and the FormIDs it references.
    RelaRecord rec;
    rec.editorId = "RelaTest";
    rec.formId = 0x00100100;
    rec.parentFormId = 0x00100050; // references StatA (owned)
    rec.childFormId = 0x00200001;  // references another plugin (not remapped)
    relaCol.add(rec);

    // Add StatA so the compactor has an owned record to remap.
    auto& statCol = data.getStatCollection();
    StatRecord a; a.editorId = "StatA"; a.formId = 0x00100050;
    statCol.add(a);

    FormIdCompactor compactor(data);
    QCOMPARE(compactor.compact(), 2);
    QCOMPARE(compactor.ownedRecordCount(), 2);

    const RelaRecord& saved = relaCol.getRecord(0).get();
    // The reference to an owned record follows the remap; the foreign
    // reference is untouched.
    const quint32 newParent = 0x00100000u; // StatA is the only stat -> local 0
    QCOMPARE(saved.parentFormId, newParent);
    QCOMPARE(saved.childFormId, static_cast<quint32>(0x00200001));
}

void TestEsl::testCompactorTooManyRecords()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& statCol = data.getStatCollection();

    for (int i = 0; i < 4097; ++i)
    {
        StatRecord r;
        r.editorId = QString("S%1").arg(i);
        r.formId = 0x00100000u + static_cast<quint32>(i);
        statCol.add(r);
    }

    FormIdCompactor compactor(data);
    QCOMPARE(compactor.compact(), -1); // exceeds the ESL ceiling
}

void TestEsl::testLightMasterFlagRoundTrip()
{
    QTemporaryFile tmpFile;
    tmpFile.open();
    const QString path = tmpFile.fileName();
    tmpFile.close();

    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        ESMWriter writer;
        writer.setFileFlags(0x200); // LightMaster
        writer.save(f);
        writer.setAuthor("ESL Test");
        writer.setVersion(1.0f);
        writer.close();
        f.close();
    }

    ESMReader reader(path);
    reader.open();
    const auto& header = reader.getHeader();
    QVERIFY(header.flags.val & 0x200);
    QVERIFY(!(header.flags.val & 0x01)); // not a master
}

QTEST_MAIN(TestEsl)
#include "test_esl.moc"
