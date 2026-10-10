#include <QTest>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "src/model/tools/plugindiff.hpp"
#include "src/model/tools/plugintextexport.hpp"
#include "src/model/tools/recordmerger.hpp"
#include "src/model/tools/pluginmerger.hpp"
#include "libs/files/esm/esmwriter.hpp"
#include "libs/files/esm/common.hpp"
#include "libs/files/log/logger.hpp"

// Raw plugin fixtures: recorded directly through ESMWriter so the tests stay
// independent of Data/Document construction and exercise exactly the bytes
// the differ and merger read.
namespace {

struct Sub { NAME name; QByteArray data; };
struct Rec { NAME type; quint32 formId; QVector<Sub> subs; };

void writeTestPlugin(const QString& path, const QVector<Rec>& records)
{
    ESMWriter writer;
    writer.setVersion(1.0f);
    QFile out(path);
    QVERIFY(out.open(QIODevice::WriteOnly));
    writer.save(out);
    for (const Rec& r : records)
    {
        RecHeader header;
        header.id = r.formId;
        writer.startRecord(r.type, header);
        for (const Sub& s : r.subs)
        {
            writer.startSubRecord(s.name);
            writer.writeRawData(s.data.constData(), s.data.size());
            writer.endSubRecord();
        }
        writer.endRecord();
    }
    writer.close();
    out.close();
}

QByteArray zstr(const QString& s)
{
    return s.toLatin1() + QByteArray(1, '\0');
}

} // namespace

class TestPluginDiff : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testIdenticalPlugins();
    void testAddedRemovedModified();
    void testTextExportDeterministic();
    void testThreeWayMerge();
    void testThreeWayConflictAndIdConflict();
    void testMergeWritesOutput();
};

void TestPluginDiff::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Warning);
    OpenCK::Logging::Logger::instance().init(
        QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_plugindiff_log.txt"));
}

void TestPluginDiff::testIdenticalPlugins()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString a = dir.filePath("a.esp");
    const QString b = dir.filePath("b.esp");
    const QVector<Rec> records = {
        { NAME('STAT'), 0x01000001,
          { { NAME('EDID'), zstr("Chair") } } },
    };
    writeTestPlugin(a, records);
    writeTestPlugin(b, records);

    const openck::PluginDiffReport report = openck::PluginDiffer::diff(a, b);
    QVERIFY2(report.ok(), qPrintable(report.error));
    QCOMPARE(report.sameRecords, 1);
    QCOMPARE(report.records.size(), 0);
    QCOMPARE(report.modifiedRecords, 0);
    QCOMPARE(report.addedRecords, 0);
    QCOMPARE(report.removedRecords, 0);
}

void TestPluginDiff::testAddedRemovedModified()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString left = dir.filePath("left.esp");
    const QString right = dir.filePath("right.esp");
    writeTestPlugin(left, {
        { NAME('STAT'), 0x01000001,
          { { NAME('EDID'), zstr("KeepMe") } } },
        { NAME('STAT'), 0x01000002,
          { { NAME('EDID'), zstr("OnlyLeft") } } },
        { NAME('STAT'), 0x01000003,
          { { NAME('EDID'), zstr("Changed") } } },
    });
    writeTestPlugin(right, {
        { NAME('STAT'), 0x01000001,
          { { NAME('EDID'), zstr("KeepMe") } } },
        { NAME('STAT'), 0x01000004,
          { { NAME('EDID'), zstr("OnlyRight") } } },
        { NAME('STAT'), 0x01000003,
          { { NAME('EDID'), zstr("EditedHere") } } },
    });

    const openck::PluginDiffReport report = openck::PluginDiffer::diff(left, right);
    QVERIFY2(report.ok(), qPrintable(report.error));
    QCOMPARE(report.removedRecords, 1);   // OnlyLeft
    QCOMPARE(report.addedRecords, 1);     // OnlyRight
    QCOMPARE(report.modifiedRecords, 1);  // Changed
    QCOMPARE(report.sameRecords, 1);      // KeepMe

    // The modified record must expose the subrecord alignment, not just a
    // "record differs" verdict: the reviewer needs to see what changed.
    const openck::RecordDiffEntry* edited = nullptr;
    for (const openck::RecordDiffEntry& e : report.records)
    {
        if (e.formId == 0x01000003)
            edited = &e;
    }
    QVERIFY(edited != nullptr);
    QCOMPARE(edited->status, openck::DiffStatus::Modified);
    QCOMPARE(edited->subs.size(), 1);
    QCOMPARE(edited->subs.first().status, openck::DiffStatus::Modified);
    QCOMPARE(edited->subs.first().left, zstr("Changed"));
    QCOMPARE(edited->subs.first().right, zstr("EditedHere"));
    QCOMPARE(edited->editorId, QString("Changed"));
}

void TestPluginDiff::testTextExportDeterministic()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString plugin = dir.filePath("plugin.esp");
    writeTestPlugin(plugin, {
        { NAME('STAT'), 0x01000002,
          { { NAME('EDID'), zstr("B") } } },
        { NAME('STAT'), 0x01000001,
          { { NAME('EDID'), zstr("A") } } },
        { NAME('NPC_'), 0x01000003,
          { { NAME('EDID'), zstr("C") } } },
    });

    const QString json1 = dir.filePath("plugin.json");
    const QString json2 = dir.filePath("plugin2.json");
    QVERIFY(openck::PluginTextExport::exportJson(plugin, json1));
    QVERIFY(openck::PluginTextExport::exportJson(plugin, json2));

    QFile f1(json1), f2(json2);
    QVERIFY(f1.open(QIODevice::ReadOnly));
    QVERIFY(f2.open(QIODevice::ReadOnly));
    // Byte-identical output for byte-identical input: this is what makes
    // the exports actually diffable under Git.
    const QByteArray bytes1 = f1.readAll();
    const QByteArray bytes2 = f2.readAll();
    QCOMPARE(bytes1, bytes2);

    const QJsonObject root = QJsonDocument::fromJson(bytes1).object();
    const QJsonObject records = root["records"].toObject();
    // Records keyed by "TYPE:formId" keep text diffs local to the record.
    QVERIFY(records.contains("STAT:01000002"));
    QVERIFY(records.contains("NPC_:01000003"));

    const QJsonObject a = records["STAT:01000001"].toObject();
    const QJsonArray subs = a["subs"].toArray();
    QCOMPARE(subs.size(), 1);
    QCOMPARE(subs.first().toObject()["name"].toString(), QString("EDID"));
}

void TestPluginDiff::testThreeWayMerge()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // Base: two statics. Mine edits the first, theirs edits the second.
    const QString base = dir.filePath("base.esp");
    const QString mine = dir.filePath("mine.esp");
    const QString theirs = dir.filePath("theirs.esp");
    const QByteArray keep = zstr("Keep");
    writeTestPlugin(base, {
        { NAME('STAT'), 0x01000001,
          { { NAME('EDID'), keep } } },
        { NAME('STAT'), 0x01000002,
          { { NAME('EDID'), zstr("Second") } } },
    });
    writeTestPlugin(mine, {
        { NAME('STAT'), 0x01000001,
          { { NAME('EDID'), zstr("MineChangedFirst") } } },
        { NAME('STAT'), 0x01000002,
          { { NAME('EDID'), zstr("Second") } } },
    });
    writeTestPlugin(theirs, {
        { NAME('STAT'), 0x01000001,
          { { NAME('EDID'), keep } } },
        { NAME('STAT'), 0x01000002,
          { { NAME('EDID'), zstr("TheirsChangedSecond") } } },
    });

    const openck::RecordMergePlan plan =
        openck::RecordMerger::merge(base, mine, theirs);
    QCOMPARE(plan.conflicts, 0);
    QCOMPARE(plan.actions.size(), 2);
    for (const openck::MergeAction& action : plan.actions)
        QCOMPARE(action.kind, openck::MergeAction::Take);

    // Both edits must survive: mine's first record and theirs' second.
    const QByteArray mergedFirst =
        plan.actions.at(0).record.subs.first().payload;
    QCOMPARE(mergedFirst, zstr("MineChangedFirst"));
    QCOMPARE(plan.actions.at(1).record.subs.first().payload,
             zstr("TheirsChangedSecond"));
}

void TestPluginDiff::testThreeWayConflictAndIdConflict()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString base = dir.filePath("base.esp");
    const QString mine = dir.filePath("mine.esp");
    const QString theirs = dir.filePath("theirs.esp");
    writeTestPlugin(base, {
        { NAME('STAT'), 0x01000001,
          { { NAME('EDID'), zstr("Shared") } } },
    });
    // Both branches edit the same record differently.
    writeTestPlugin(mine, {
        { NAME('STAT'), 0x01000001,
          { { NAME('EDID'), zstr("MineEdit") } } },
    });
    writeTestPlugin(theirs, {
        { NAME('STAT'), 0x01000001,
          { { NAME('EDID'), zstr("TheirsEdit") } } },
    });

    const openck::RecordMergePlan plan =
        openck::RecordMerger::merge(base, mine, theirs);
    QCOMPARE(plan.conflicts, 1);
    QCOMPARE(plan.actions.size(), 1);
    QCOMPARE(plan.actions.first().kind, openck::MergeAction::Conflict);
    // The working side is kept; the reviewer resolves it in the editor.
    QCOMPARE(plan.actions.first().record.subs.first().payload, zstr("MineEdit"));

    // Both branches allocate the same FormID for different objects.
    const QString base2 = dir.filePath("base2.esp");
    const QString mine2 = dir.filePath("mine2.esp");
    const QString theirs2 = dir.filePath("theirs2.esp");
    writeTestPlugin(base2, {
        { NAME('STAT'), 0x01000001,
          { { NAME('EDID'), zstr("Existing") } } },
    });
    writeTestPlugin(mine2, {
        { NAME('STAT'), 0x01000001,
          { { NAME('EDID'), zstr("Existing") } } },
        { NAME('NPC_'), 0x01000002,
          { { NAME('EDID'), zstr("MineGuard") } } },
    });
    writeTestPlugin(theirs2, {
        { NAME('STAT'), 0x01000001,
          { { NAME('EDID'), zstr("Existing") } } },
        { NAME('NPC_'), 0x01000002,
          { { NAME('EDID'), zstr("TheirsGuard") } } },
    });

    const openck::RecordMergePlan idPlan =
        openck::RecordMerger::merge(base2, mine2, theirs2);
    QCOMPARE(idPlan.idConflicts, 1);
    QCOMPARE(idPlan.conflicts, 1);
}

void TestPluginDiff::testMergeWritesOutput()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString base = dir.filePath("base.esp");
    const QString mine = dir.filePath("mine.esp");
    const QString theirs = dir.filePath("theirs.esp");
    const QString output = dir.filePath("merged.esp");
    writeTestPlugin(base, {
        { NAME('STAT'), 0x01000001,
          { { NAME('EDID'), zstr("One") } } },
    });
    writeTestPlugin(mine, {
        { NAME('STAT'), 0x01000001,
          { { NAME('EDID'), zstr("One") } } },
        { NAME('STAT'), 0x01000002,
          { { NAME('EDID'), zstr("MineOnly") } } },
    });
    writeTestPlugin(theirs, {
        { NAME('STAT'), 0x01000001,
          { { NAME('EDID'), zstr("One") } } },
        { NAME('STAT'), 0x01000003,
          { { NAME('EDID'), zstr("TheirsOnly") } } },
    });

    const openck::MergeWriteReport report =
        openck::PluginMerger::mergeToFile(base, mine, theirs, output);
    QVERIFY2(report.ok(), qPrintable(report.error));
    QCOMPARE(report.conflictsRetained, 0);
    QCOMPARE(report.recordsWritten, 3);

    // The merged plugin reads back as a plugin: both branches' additions
    // survive with their payloads intact. Diffing mine -> output shows the
    // theirs-only addition as Added; the reverse would hide a dropped record.
    const openck::PluginDiffReport against =
        openck::PluginDiffer::diff(mine, output);
    QVERIFY2(against.ok(), qPrintable(against.error));
    QCOMPARE(against.records.size(), 1);
    if (against.records.size() == 1)
    {
        const openck::RecordDiffEntry& only = against.records.first();
        QCOMPARE(only.status, openck::DiffStatus::Added);
        QCOMPARE(only.formId, 0x01000003u);
        QCOMPARE(only.editorId, QString("TheirsOnly"));
    }
}

QTEST_APPLESS_MAIN(TestPluginDiff)
#include "test_plugindiff.moc"
