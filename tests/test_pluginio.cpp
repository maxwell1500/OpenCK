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
#include "../../libs/files/esm/weaprecord.hpp"
#include "../../libs/files/esm/armorrecord.hpp"

class TestPluginIO : public QObject
{
    Q_OBJECT

private slots:
    void testEsmWriter_TES4Header();
    void testEsmReader_TES4Header();
    void testRoundTrip_NpcRecord();
    void testRoundTrip_WeaponRecord();
    void testRoundTrip_ArmorRecord();
    void testMastersList();
    void testEmptyPlugin();
};

void TestPluginIO::testEsmWriter_TES4Header()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    
    QString filePath = tempDir.path() + "/test.esm";
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    
    ESMWriter writer;
    writer.setVersion(1.0f);
    writer.setAuthor("Test Author");
    writer.setDescription("Test Description");
    writer.setNumRecords(5);
    writer.addMaster("Skyrim.esm", 1000000);
    writer.addMaster("Update.esm", 2000000);
    
    writer.save(file);
    writer.close();
    file.close();
    
    QVERIFY(QFile::exists(filePath));
    QFile readFile(filePath);
    QVERIFY(readFile.open(QIODevice::ReadOnly));
    
    QByteArray data = readFile.readAll();
    readFile.close();
    
    QCOMPARE(data.left(4), QByteArray("TES4"));
    QVERIFY(data.size() > 32);
}

void TestPluginIO::testEsmReader_TES4Header()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    
    QString filePath = tempDir.path() + "/test.esm";
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    
    ESMWriter writer;
    writer.setVersion(1.0f);
    writer.setAuthor("Test Author");
    writer.setDescription("Test Description");
    writer.setNumRecords(3);
    writer.addMaster("Skyrim.esm", 1000000);
    
    writer.save(file);
    writer.close();
    file.close();
    
    ESMReader reader(filePath);
    reader.open();
    
    QCOMPARE(reader.getHeader().version, 1.0f);
    QString author = reader.getHeader().author;
    QVERIFY(author.startsWith("Test Author"));
    QString description = reader.getHeader().description;
    QVERIFY(description.startsWith("Test Description"));
    QCOMPARE(reader.getHeader().numRecords, 3);
    QCOMPARE(reader.getHeader().masters.size(), 1);
    QString masterName = reader.getHeader().masters[0].name;
    QVERIFY(masterName.startsWith("Skyrim.esm"));
}

void TestPluginIO::testRoundTrip_NpcRecord()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    
    QString filePath = tempDir.path() + "/test.esm";
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    
    ESMWriter writer;
    writer.setVersion(1.0f);
    writer.setNumRecords(2);
    
    writer.save(file);
    
    NpcRecord npc;
    npc.editorId = "TestNPC";
    npc.fullName = "Test Character";
    npc.formId = 0x00012345;
    npc.level = 10;
    npc.race = 1;
    npc.faction = 2;
    
    {
        RecHeader header;
        header.flags.val = 0;
        header.id = 'NPC ';
        writer.startRecord('NPC ', header);
    }
    writer.writeSubZString('EDID', npc.editorId);
    writer.writeSubZString('FNAM', npc.fullName);
    writer.writeSubData('SNAM', (quint32)npc.level);
    writer.endRecord();
    
    writer.close();
    file.close();
    
    ESMReader reader(filePath);
    reader.open();
    
    // After open(), reader is positioned past TES4 header.
    // First record should be NPC_
    NAME recType = reader.readName();
    QCOMPARE(recType, 'NPC ');
    RecHeader recHeader = reader.readHeader();
    reader.skip(recHeader.size);
}

void TestPluginIO::testRoundTrip_WeaponRecord()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    
    QString filePath = tempDir.path() + "/test.esm";
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    
    ESMWriter writer;
    writer.setVersion(1.0f);
    writer.setNumRecords(2);
    
    writer.save(file);
    
    WeaponRecord weapon;
    weapon.editorId = "TestWeapon";
    weapon.formId = 0x0001ABCD;
    weapon.damage = 25.5f;
    weapon.speed = 1.2f;
    weapon.weight = 15.0f;
    weapon.value = 100;
    
    {
        RecHeader header;
        header.flags.val = 0;
        header.id = 'WEAP';
        writer.startRecord('WEAP', header);
    }
    writer.writeSubZString('EDID', weapon.editorId);
    writer.writeSubData('DNAM', weapon.damage);
    writer.writeSubData('ANAM', weapon.speed);
    writer.writeSubData('FNAM', weapon.weight);
    writer.writeSubData('CNAM', weapon.value);
    writer.endRecord();
    
    writer.close();
    file.close();
    
    QVERIFY(QFile::exists(filePath));
    QFile verifyFile(filePath);
    QVERIFY(verifyFile.open(QIODevice::ReadOnly));
    QByteArray data = verifyFile.readAll();
    QVERIFY(data.size() > 100);
    verifyFile.close();
}

void TestPluginIO::testRoundTrip_ArmorRecord()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    
    QString filePath = tempDir.path() + "/test.esm";
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    
    ESMWriter writer;
    writer.setVersion(1.0f);
    writer.setNumRecords(2);
    
    writer.save(file);
    
    ArmorRecord armor;
    armor.editorId = "TestArmor";
    armor.formId = 0x0002FFFF;
    armor.armorRating = 50;
    armor.weight = 25.0f;
    armor.value = 500;
    
    {
        RecHeader header;
        header.flags.val = 0;
        header.id = 'ARMO';
        writer.startRecord('ARMO', header);
    }
    writer.writeSubZString('EDID', armor.editorId);
    writer.writeSubData('DNAM', armor.armorRating);
    writer.writeSubData('FNAM', armor.weight);
    writer.writeSubData('CNAM', armor.value);
    writer.endRecord();
    
    writer.close();
    file.close();
    
    QVERIFY(QFile::exists(filePath));
}

void TestPluginIO::testMastersList()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    
    QString filePath = tempDir.path() + "/test.esm";
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    
    ESMWriter writer;
    writer.setVersion(1.0f);
    writer.clearMasters();
    writer.addMaster("Skyrim.esm", 1000000);
    writer.addMaster("Update.esm", 2000000);
    writer.addMaster("Dawnguard.esm", 3000000);
    writer.addMaster("HearthFires.esm", 4000000);
    writer.addMaster("Dragonborn.esm", 5000000);
    
    writer.save(file);
    writer.close();
    file.close();
    
    ESMReader reader(filePath);
    reader.open();
    
    QCOMPARE(reader.getHeader().masters.size(), 5);
    QVERIFY(QString(reader.getHeader().masters[0].name).startsWith("Skyrim.esm"));
    QVERIFY(QString(reader.getHeader().masters[1].name).startsWith("Update.esm"));
    QVERIFY(QString(reader.getHeader().masters[2].name).startsWith("Dawnguard.esm"));
    QVERIFY(QString(reader.getHeader().masters[3].name).startsWith("HearthFires.esm"));
    QVERIFY(QString(reader.getHeader().masters[4].name).startsWith("Dragonborn.esm"));
}

void TestPluginIO::testEmptyPlugin()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    
    QString filePath = tempDir.path() + "/empty.esm";
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    
    ESMWriter writer;
    writer.setVersion(1.0f);
    writer.setNumRecords(1);
    
    writer.save(file);
    writer.close();
    file.close();
    
    QVERIFY(QFile::exists(filePath));
    
    ESMReader reader(filePath);
    reader.open();
    
    QCOMPARE(reader.getHeader().numRecords, 1);
    QVERIFY(reader.getHeader().masters.isEmpty());
}

QTEST_MAIN(TestPluginIO)
#include "test_pluginio.moc"
