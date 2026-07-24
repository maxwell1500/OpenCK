#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDataStream>
#include <QByteArray>

#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/esm/tes4.hpp"
#include "../../libs/files/esm/common.hpp"
#include "../../libs/files/esm/npcrecord.hpp"
#include "../../model/world/idcollection.hpp"

class TestConflictDetection : public QObject
{
    Q_OBJECT

private slots:
    void testSameEditorId_DifferentPlugins();
    void testDifferentEditorId_SamePlugin();
    void testFormId_PluginIndex();
    void testMultipleConflicts_SameType();
    void testNoConflicts_UniqueIds();
};

void TestConflictDetection::testSameEditorId_DifferentPlugins()
{
    // Create two collections simulating different plugins
    IdCollection<NpcRecord> pluginA;
    IdCollection<NpcRecord> pluginB;
    
    // Same editor ID in both plugins (conflict)
    NpcRecord npcA;
    npcA.editorId = "sharednpc";
    npcA.formId = 0x00012345; // Plugin index 1
    npcA.fullName = "From Plugin A";
    
    NpcRecord npcB;
    npcB.editorId = "sharednpc";
    npcB.formId = 0x00022345; // Plugin index 2
    npcB.fullName = "From Plugin B";
    
    pluginA.add(npcA);
    pluginB.add(npcB);
    
    // Verify both have the same editor ID
    QCOMPARE(pluginA.getId(0), QString("sharednpc"));
    QCOMPARE(pluginB.getId(0), QString("sharednpc"));
    
    // Verify different plugin indices in FormID
    quint32 formIdA = npcA.formId;
    quint32 formIdB = npcB.formId;
    
    int pluginIndexA = (formIdA >> 16) & 0xFFFF;
    int pluginIndexB = (formIdB >> 16) & 0xFFFF;
    
    QCOMPARE(pluginIndexA, 1);
    QCOMPARE(pluginIndexB, 2);
}

void TestConflictDetection::testDifferentEditorId_SamePlugin()
{
    IdCollection<NpcRecord> collection;
    
    NpcRecord npc1;
    npc1.editorId = "NPC_One";
    npc1.formId = 0x00012345;
    
    NpcRecord npc2;
    npc2.editorId = "NPC_Two";
    npc2.formId = 0x00012346;
    
    collection.add(npc1);
    collection.add(npc2);
    
    // Verify different editor IDs
    QVERIFY(collection.getId(0) != collection.getId(1));
    
    // Both in same plugin (same plugin index)
    quint32 formId1 = npc1.formId;
    quint32 formId2 = npc2.formId;
    
    int pluginIndex1 = (formId1 >> 16) & 0xFFFF;
    int pluginIndex2 = (formId2 >> 16) & 0xFFFF;
    
    QCOMPARE(pluginIndex1, pluginIndex2);
}

void TestConflictDetection::testFormId_PluginIndex()
{
    // Test FormID structure: XXPPnnnn where XX=plugin index, PP=record type, nnnn=record number
    
    // Plugin index 1 (Skyrim.esm)
    quint32 formId1 = 0x00012345;
    QCOMPARE((formId1 >> 24) & 0xFF, 0x00); // First byte is 00 for player plugins
    QCOMPARE((formId1 >> 16) & 0xFFFF, 0x0001); // Plugin index
    
    // Plugin index 2 (Update.esm)
    quint32 formId2 = 0x0002ABCD;
    QCOMPARE((formId2 >> 16) & 0xFFFF, 0x0002);
    
    // Plugin index 14 (player plugin)
    quint32 formId3 = 0x0701FFFF;
    QCOMPARE((formId3 >> 16) & 0xFFFF, 0x0701);
}

void TestConflictDetection::testMultipleConflicts_SameType()
{
    IdCollection<NpcRecord> pluginA;
    IdCollection<NpcRecord> pluginB;
    
    // Multiple conflicts
    QString conflictIds[] = {"npc_a", "npc_b", "npc_c"};
    
    for (int i = 0; i < 3; i++)
    {
        NpcRecord npcA;
        npcA.editorId = conflictIds[i];
        npcA.formId = 0x00010000 + i;
        pluginA.add(npcA);
        
        NpcRecord npcB;
        npcB.editorId = conflictIds[i];
        npcB.formId = 0x00020000 + i;
        pluginB.add(npcB);
    }
    
    // Verify all conflicts exist in both plugins
    for (int i = 0; i < 3; i++)
    {
        QVERIFY(pluginA.searchId(conflictIds[i]) >= 0);
        QVERIFY(pluginB.searchId(conflictIds[i]) >= 0);
    }
}

void TestConflictDetection::testNoConflicts_UniqueIds()
{
    IdCollection<NpcRecord> pluginA;
    IdCollection<NpcRecord> pluginB;
    
    // Unique IDs in each plugin (no conflicts)
    NpcRecord npcA;
    npcA.editorId = "onlyina";
    npcA.formId = 0x00012345;
    pluginA.add(npcA);
    
    NpcRecord npcB;
    npcB.editorId = "onlyinb";
    npcB.formId = 0x00022345;
    pluginB.add(npcB);
    
    // Verify no shared IDs
    QVERIFY(pluginA.searchId("onlyinb") < 0);
    QVERIFY(pluginB.searchId("onlyina") < 0);
}

QTEST_MAIN(TestConflictDetection)
#include "test_conflict.moc"
