#include <QtTest>

#include "../../src/model/tools/plugincompactor.hpp"
#include "../../src/model/world/irecordcollection.hpp"
#include "../../src/model/world/collection.hpp"
#include "../../src/model/world/idcollection.hpp"
#include "../../src/model/world/basecollection.hpp"
#include "../../libs/files/esm/refrecord.hpp"
#include "../../libs/files/esm/cellrecord.hpp"

class TestPluginCompactor : public QObject
{
    Q_OBJECT

private slots:
    void testCollectFormIds();
    void testBuildMapDenseRenumber();
    void testBuildMapPreservesMasterByte();
    void testRemapUnmapped();
    void testRepointRefr();
    void testRepointCell();
    void testRenumberRecordsViaCollection();
};

void TestPluginCompactor::testCollectFormIds()
{
    IdCollection<RefrRecord> refs;
    RefrRecord r1; r1.editorId = "R1"; r1.formId = 0x00000400; r1.baseId = 0x00001234;
    RefrRecord r2; r2.editorId = "R2"; r2.formId = 0x00000401; r2.baseId = 0x00000400;
    RefrRecord dup; dup.editorId = "R3"; dup.formId = 0x00000400;
    refs.add(r1);
    refs.add(r2);
    refs.add(dup);

    QVector<const IRecordCollection*> collections = { &refs };
    const QVector<quint32> ids = PluginCompactor::collectFormIds(collections);

    // 0x400 and 0x401 collected; the duplicate is deduped.
    QCOMPARE(ids.size(), 2);
    QVERIFY(ids.contains(0x00000400u));
    QVERIFY(ids.contains(0x00000401u));
}

void TestPluginCompactor::testBuildMapDenseRenumber()
{
    QVector<quint32> ids = { 0x00000050, 0x00000020, 0x00000090 };
    const PluginCompactor::RenumberMap map = PluginCompactor::buildMap(ids);

    QCOMPARE(map.renumbered, 3);
    QCOMPARE(map.oldToNew.size(), 3);
    // Local IDs are packed densely starting at 1, sorted by old ID.
    QCOMPARE(map.oldToNew.value(0x00000020), 0x00000001u);
    QCOMPARE(map.oldToNew.value(0x00000050), 0x00000002u);
    QCOMPARE(map.oldToNew.value(0x00000090), 0x00000003u);
}

void TestPluginCompactor::testBuildMapPreservesMasterByte()
{
    QVector<quint32> ids = { 0x02000100, 0x02000400 };
    const PluginCompactor::RenumberMap map = PluginCompactor::buildMap(ids);

    QCOMPARE(map.renumbered, 2);
    QCOMPARE(map.oldToNew.value(0x02000100), 0x02000001u);
    QCOMPARE(map.oldToNew.value(0x02000400), 0x02000002u);
}

void TestPluginCompactor::testRemapUnmapped()
{
    QVector<quint32> ids = { 0x00000010 };
    const PluginCompactor::RenumberMap map = PluginCompactor::buildMap(ids);

    // A master-owned ID (not in the map) is left untouched.
    QCOMPARE(PluginCompactor::remap(map, 0x01000001), 0x01000001u);
    // An ID in the map is renumbered.
    QCOMPARE(PluginCompactor::remap(map, 0x00000010), 0x00000001u);
}

void TestPluginCompactor::testRepointRefr()
{
    QVector<quint32> ids = { 0x00000400, 0x00000401, 0x00000402 };
    const PluginCompactor::RenumberMap map = PluginCompactor::buildMap(ids);

    RefrRecord refr;
    refr.formId = 0x00000400;
    refr.baseId = 0x00000402;   // targets a renumbered record
    refr.owner = 0x01000001;    // master-owned -> unchanged
    refr.scriptIds = { 0x00000401, 0x00000999 };  // one renumbered, one external

    PluginCompactor::Result result;
    result.totalRecords = 1;
    PluginCompactor::repointRefr(refr, map, result);

    QCOMPARE(refr.baseId, 0x00000003u);
    QCOMPARE(refr.owner, 0x01000001u);
    QCOMPARE(refr.scriptIds.size(), 2);
    QCOMPARE(refr.scriptIds[0], 0x00000002u);
    QCOMPARE(refr.scriptIds[1], 0x00000999u);
    // renumberId counts each changed ID; repointRefr counts repointed refs.
    QCOMPARE(result.repointedReferences, 2); // base + 1 script (owner unchanged)
    QCOMPARE(result.renumbered, 2);
}

void TestPluginCompactor::testRepointCell()
{
    QVector<quint32> ids = { 0x00000001 };
    const PluginCompactor::RenumberMap map = PluginCompactor::buildMap(ids);

    CellRecord cell;
    cell.formId = 0x00000001;
    cell.owner = 0x00000001;

    PluginCompactor::Result result;
    PluginCompactor::repointCell(cell, map, result);

    QCOMPARE(cell.owner, 0x00000001u); // renumber of self is identity here
    QCOMPARE(result.repointedReferences, 0);
}

void TestPluginCompactor::testRenumberRecordsViaCollection()
{
    IdCollection<RefrRecord> refs;
    RefrRecord r1; r1.editorId = "R1"; r1.formId = 0x00000100; r1.baseId = 0x00000101;
    RefrRecord r2; r2.editorId = "R2"; r2.formId = 0x00000101; r2.baseId = 0x00000100;
    refs.add(r1);
    refs.add(r2);

    QVector<const IRecordCollection*> collections = { &refs };
    const QVector<quint32> ids = PluginCompactor::collectFormIds(collections);
    const PluginCompactor::RenumberMap map = PluginCompactor::buildMap(ids);

    // Renumber the records' own form IDs through the generic interface.
    for (int i = 0; i < refs.count(); ++i)
    {
        const quint32 oldId = refs.getFormId(i);
        refs.setFormId(i, PluginCompactor::remap(map, oldId));
    }

    QCOMPARE(refs.getFormId(0), 0x00000001u);
    QCOMPARE(refs.getFormId(1), 0x00000002u);
}

QTEST_MAIN(TestPluginCompactor)
#include "test_plugincompactor.moc"
