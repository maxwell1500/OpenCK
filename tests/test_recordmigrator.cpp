#include <QTest>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <memory>

#include "src/model/tools/recordmigrator.hpp"
#include "src/model/world/data.hpp"
#include "src/model/world/collection.hpp"
#include "libs/files/esm/statrecord.hpp"
#include "libs/files/esm/npcrecord.hpp"
#include "libs/files/log/logger.hpp"

class TestRecordMigrator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testSharedCodes();
    void testMigrateStatRecords();
    void testCollisionRenames();
    void testUnknownCodeReported();
};

static std::unique_ptr<Data> makeData(GameFormat::Game game, const FilePaths& paths)
{
    auto data = std::make_unique<Data>(QStringList(), paths);
    QVector<MasterData> masters;
    data->configureNewFile(game, masters, QStringLiteral("test"), 0x800);
    return data;
}

void TestRecordMigrator::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Warning);
    OpenCK::Logging::Logger::instance().init(QStringLiteral(
        "C:/Users/max/AppData/Local/Temp/opencode/test_recordmigrator_log.txt"));
}

void TestRecordMigrator::testSharedCodes()
{
    const QVector<QString> skyrimToStarfield =
        RecordMigrator::sharedRecordCodes(GameFormat::Game::Skyrim,
                                          GameFormat::Game::Starfield);
    // The TES4 core is shared: STAT, NPC_, and friends move freely.
    QVERIFY(skyrimToStarfield.contains(QStringLiteral("STAT")));
    QVERIFY(skyrimToStarfield.contains(QStringLiteral("NPC_")));
    QVERIFY(skyrimToStarfield.contains(QStringLiteral("CELL")));
    // Morrowind's records ride the generic Tes3Record path, so a TES3 side
    // has no shared codes with the TES4 family.
    QVERIFY(RecordMigrator::sharedRecordCodes(GameFormat::Game::Morrowind,
                                              GameFormat::Game::Skyrim)
                .isEmpty());
}

void TestRecordMigrator::testMigrateStatRecords()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    FilePaths paths;
    paths.dataDir.setPath(dir.path());

    auto source = makeData(GameFormat::Game::Skyrim, paths);
    auto dest = makeData(GameFormat::Game::Starfield, paths);

    StatRecord stat;
    stat.formId = 0x05001234;
    stat.editorId = QStringLiteral("Chair");
    source->getStatCollection().add(stat);

    NpcRecord npc;
    npc.formId = 0x05001235;
    npc.editorId = QStringLiteral("Guard");
    source->getNpcCollection().add(npc);

    const MigrationReport report = RecordMigrator::migrate(*source, *dest);
    QVERIFY2(report.migrated.size() == 2,
             qPrintable(QString("skipped: %1").arg(report.skipped.join("; "))));
    QCOMPARE(report.from, GameFormat::Game::Skyrim);
    QCOMPARE(report.to, GameFormat::Game::Starfield);

    // The destination owns the records now: same editor IDs, its own space.
    int statIndex = dest->getStatCollection().searchId(QStringLiteral("Chair"));
    QVERIFY(statIndex >= 0);
    QCOMPARE(dest->getStatCollection().getEditorId(statIndex),
             QStringLiteral("Chair"));
    QVERIFY(dest->getNpcCollection().searchId(QStringLiteral("Guard")) >= 0);

    // The source is untouched.
    QVERIFY(source->getStatCollection().searchId(QStringLiteral("Chair")) >= 0);
    QCOMPARE(source->getStatCollection().size(), 1);
}

void TestRecordMigrator::testCollisionRenames()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    FilePaths paths;
    paths.dataDir.setPath(dir.path());

    auto source = makeData(GameFormat::Game::Skyrim, paths);
    auto dest = makeData(GameFormat::Game::Starfield, paths);

    StatRecord stat;
    stat.formId = 0x05001234;
    stat.editorId = QStringLiteral("Chair");
    source->getStatCollection().add(stat);

    // The destination already owns a record by that name; the migrated copy
    // must not overwrite it.
    StatRecord existing;
    existing.formId = 0x05009999;
    existing.editorId = QStringLiteral("Chair");
    dest->getStatCollection().add(existing);

    const MigrationReport report = RecordMigrator::migrate(*source, *dest);
    QCOMPARE(report.migrated.size(), 1);
    QCOMPARE(report.renamed, 1);
    QCOMPARE(report.migrated.first().editorId, QStringLiteral("Chair_migrated1"));
    // Both survive.
    QCOMPARE(dest->getStatCollection().size(), 2);
    QVERIFY(dest->getStatCollection().searchId(QStringLiteral("Chair")) >= 0);
    QVERIFY(dest->getStatCollection().searchId(
                QStringLiteral("Chair_migrated1")) >= 0);
}

void TestRecordMigrator::testUnknownCodeReported()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    FilePaths paths;
    paths.dataDir.setPath(dir.path());

    auto source = makeData(GameFormat::Game::Skyrim, paths);
    auto dest = makeData(GameFormat::Game::Starfield, paths);

    // A code the migrator does not carry is reported, not silently dropped.
    const MigrationReport report =
        RecordMigrator::migrate(*source, *dest, QStringList{ QStringLiteral("ZZZZ") });
    QVERIFY(!report.ok());
    QCOMPARE(report.skipped.size(), 1);
    QVERIFY(report.skipped.first().contains(QStringLiteral("ZZZZ")));
}

QTEST_MAIN(TestRecordMigrator)
#include "test_recordmigrator.moc"
