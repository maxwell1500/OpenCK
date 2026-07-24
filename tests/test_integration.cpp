#include <QtTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
#include <QStringList>

#include "../../src/model/world/idcollection.hpp"
#include "../../src/model/world/record.hpp"
#include "../../libs/files/esm/npcrecord.hpp"
#include "../../libs/files/esm/weaprecord.hpp"
#include "../../libs/files/esm/armorrecord.hpp"
#include "../../libs/files/esm/spellrecord.hpp"
#include "../../libs/files/esm/questrecord.hpp"
#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/esmwriter.hpp"

class TestIntegration : public QObject
{
    Q_OBJECT

private slots:
    void testCollection_AddMultipleRecords();
    void testCollection_GetId();
    void testCollection_SearchId();
    void testJsonDocument_Creation();
    void testMultiRecordPlugin_RoundTrip();
};

void TestIntegration::testCollection_AddMultipleRecords()
{
    IdCollection<NpcRecord> collection;
    
    for (int i = 0; i < 10; i++)
    {
        NpcRecord npc;
        npc.editorId = QString("NPC_%1").arg(i);
        npc.fullName = QString("Character %1").arg(i);
        npc.formId = 0x00010000 + i;
        npc.level = i * 5;
        
        collection.add(npc);
    }
    
    QCOMPARE(collection.size(), 10);
}

void TestIntegration::testCollection_GetId()
{
    IdCollection<NpcRecord> collection;
    
    NpcRecord npc1;
    npc1.editorId = "Dragon";
    npc1.formId = 0x00012345;
    
    NpcRecord npc2;
    npc2.editorId = "Merchant";
    npc2.formId = 0x00012346;
    
    collection.add(npc1);
    collection.add(npc2);
    
    QCOMPARE(collection.getId(0), QString("Dragon"));
    QCOMPARE(collection.getId(1), QString("Merchant"));
}

void TestIntegration::testCollection_SearchId()
{
    IdCollection<NpcRecord> collection;
    
    NpcRecord npc1;
    npc1.editorId = "dragon";
    npc1.formId = 0x00012345;
    
    NpcRecord npc2;
    npc2.editorId = "dragonknight";
    npc2.formId = 0x00012346;
    
    NpcRecord npc3;
    npc3.editorId = "merchant";
    npc3.formId = 0x00012347;
    
    collection.add(npc1);
    collection.add(npc2);
    collection.add(npc3);
    
    QCOMPARE(collection.searchId("dragon"), 0);
    QCOMPARE(collection.searchId("dragonknight"), 1);
    QCOMPARE(collection.searchId("merchant"), 2);
    QCOMPARE(collection.searchId("nonexistent"), -1);
}

void TestIntegration::testJsonDocument_Creation()
{
    QJsonObject root;
    root["test"] = "value";
    root["number"] = 42;
    
    QJsonArray array;
    array.append(1);
    array.append(2);
    array.append(3);
    root["array"] = array;
    
    QJsonDocument doc(root);
    QVERIFY(!doc.isNull());
    QVERIFY(doc.isObject());
    
    QCOMPARE(doc.object()["test"].toString(), QString("value"));
    QCOMPARE(doc.object()["number"].toInt(), 42);
    QCOMPARE(doc.object()["array"].toArray().size(), 3);
}

void TestIntegration::testMultiRecordPlugin_RoundTrip()
{
    // Test NPC_ round-trip
    {
        QTemporaryFile tmpFile;
        tmpFile.open();
        QString path = tmpFile.fileName();
        tmpFile.close();

        {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            ESMWriter writer;
            writer.setAuthor("Test");
            writer.save(file);
            RecHeader recHeader;
            recHeader.id = 0x01;
            writer.startRecord('NPC_', recHeader);
            NpcRecord r;
            r.editorId = "TestNPC";
            r.formId = 0x01;
            r.save(writer);
            writer.endRecord();
            writer.close();
            file.close();
        }

        ESMReader reader(path);
        reader.open();
        quint32 type = reader.readName();
        QCOMPARE(type, static_cast<quint32>('NPC_'));
        NpcRecord r;
        r.load(reader, true);
        QVERIFY(r.editorId.startsWith("TestNPC"));
        QCOMPARE(r.formId, static_cast<quint32>(0x01));
    }

    // Test WEAP round-trip
    {
        QTemporaryFile tmpFile;
        tmpFile.open();
        QString path = tmpFile.fileName();
        tmpFile.close();

        {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            ESMWriter writer;
            writer.setAuthor("Test");
            writer.save(file);
            RecHeader recHeader;
            recHeader.id = 0x02;
            writer.startRecord('WEAP', recHeader);
            WeaponRecord r;
            r.editorId = "TestWeapon";
            r.formId = 0x02;
            r.save(writer);
            writer.endRecord();
            writer.close();
            file.close();
        }

        ESMReader reader(path);
        reader.open();
        quint32 type = reader.readName();
        QCOMPARE(type, static_cast<quint32>('WEAP'));
        WeaponRecord r;
        r.load(reader, true);
        QVERIFY(r.editorId.startsWith("TestWeapon"));
    }

    // Test ARMO round-trip
    {
        QTemporaryFile tmpFile;
        tmpFile.open();
        QString path = tmpFile.fileName();
        tmpFile.close();

        {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            ESMWriter writer;
            writer.setAuthor("Test");
            writer.save(file);
            RecHeader recHeader;
            recHeader.id = 0x03;
            writer.startRecord('ARMO', recHeader);
            ArmorRecord r;
            r.editorId = "TestArmor";
            r.formId = 0x03;
            r.save(writer);
            writer.endRecord();
            writer.close();
            file.close();
        }

        ESMReader reader(path);
        reader.open();
        quint32 type = reader.readName();
        QCOMPARE(type, static_cast<quint32>('ARMO'));
        ArmorRecord r;
        r.load(reader, true);
        QVERIFY(r.editorId.startsWith("TestArmor"));
        QCOMPARE(r.formId, static_cast<quint32>(0x03));
    }

    // Test SPEL round-trip
    {
        QTemporaryFile tmpFile;
        tmpFile.open();
        QString path = tmpFile.fileName();
        tmpFile.close();

        {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            ESMWriter writer;
            writer.setAuthor("Test");
            writer.save(file);
            RecHeader recHeader;
            recHeader.id = 0x04;
            writer.startRecord('SPEL', recHeader);
            SpellRecord r;
            r.editorId = "TestSpell";
            r.formId = 0x04;
            r.save(writer);
            writer.endRecord();
            writer.close();
            file.close();
        }

        ESMReader reader(path);
        reader.open();
        quint32 type = reader.readName();
        QCOMPARE(type, static_cast<quint32>('SPEL'));
        SpellRecord r;
        r.load(reader, true);
        QVERIFY(r.editorId.startsWith("TestSpell"));
        QCOMPARE(r.formId, static_cast<quint32>(0x04));
    }

    // Test QUST round-trip
    {
        QTemporaryFile tmpFile;
        tmpFile.open();
        QString path = tmpFile.fileName();
        tmpFile.close();

        {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            ESMWriter writer;
            writer.setAuthor("Test");
            writer.save(file);
            RecHeader recHeader;
            recHeader.id = 0x05;
            writer.startRecord('QUST', recHeader);
            QuestRecord r;
            r.editorId = "TestQuest";
            r.formId = 0x05;
            r.save(writer);
            writer.endRecord();
            writer.close();
            file.close();
        }

        ESMReader reader(path);
        reader.open();
        quint32 type = reader.readName();
        QCOMPARE(type, static_cast<quint32>('QUST'));
        QuestRecord r;
        r.load(reader, true);
        QVERIFY(r.editorId.startsWith("TestQuest"));
        QCOMPARE(r.formId, static_cast<quint32>(0x05));
    }
}

QTEST_MAIN(TestIntegration)
#include "test_integration.moc"
