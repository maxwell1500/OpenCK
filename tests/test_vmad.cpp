#include <QtTest>
#include <QTemporaryDir>
#include <QFile>

#include "esm/vmad.hpp"
#include "esm/Questrecord.hpp"
#include "esm/Inforecord.hpp"
#include "esm/esmreader.hpp"
#include "esm/esmwriter.hpp"

class TestVmad : public QObject
{
    Q_OBJECT

private slots:
    void testPackUnpackProperties();
    void testQuestRecordVmadRoundTrip();
    void testInfoRecordVmadRoundTrip();
    void testRealSkyrimVmadBytes();
};

void TestVmad::testPackUnpackProperties()
{
    VmadData vmad;
    vmad.version = 5;
    vmad.objectFormat = 2;

    AttachedScript s1;
    s1.scriptName = QStringLiteral("MyCustomScript");
    s1.status = 0;

    // Object property
    ScriptProperty pObj;
    pObj.name = QStringLiteral("targetRef");
    pObj.type = ScriptProperty::Object;
    pObj.objectVal.formId = 0x00012345;
    pObj.objectVal.alias = 1;
    s1.properties.append(pObj);

    // Int property
    ScriptProperty pInt;
    pInt.name = QStringLiteral("count");
    pInt.type = ScriptProperty::Int;
    pInt.intVal = 42;
    s1.properties.append(pInt);

    // Float property
    ScriptProperty pFloat;
    pFloat.name = QStringLiteral("speed");
    pFloat.type = ScriptProperty::Float;
    pFloat.floatVal = 3.14159f;
    s1.properties.append(pFloat);

    // Bool property
    ScriptProperty pBool;
    pBool.name = QStringLiteral("isActive");
    pBool.type = ScriptProperty::Bool;
    pBool.boolVal = true;
    s1.properties.append(pBool);

    // String property
    ScriptProperty pStr;
    pStr.name = QStringLiteral("label");
    pStr.type = ScriptProperty::String;
    pStr.stringVal = QStringLiteral("Hello Skyrim");
    s1.properties.append(pStr);

    // Array Object property
    ScriptProperty pArrObj;
    pArrObj.name = QStringLiteral("items");
    pArrObj.type = ScriptProperty::ArrayObject;
    ScriptProperty::ObjectValue ov1, ov2;
    ov1.formId = 0x000A1111; ov1.alias = 0;
    ov2.formId = 0x000B2222; ov2.alias = 2;
    pArrObj.arrayObject = { ov1, ov2 };
    s1.properties.append(pArrObj);

    // Array Int property
    ScriptProperty pArrInt;
    pArrInt.name = QStringLiteral("levels");
    pArrInt.type = ScriptProperty::ArrayInt;
    pArrInt.arrayInt = { 1, 5, 10, 20, 50 };
    s1.properties.append(pArrInt);

    vmad.scripts.append(s1);

    const QByteArray packed = VmadData::pack(vmad);
    QVERIFY(!packed.isEmpty());

    VmadData unpacked;
    QVERIFY(VmadData::unpack(packed, unpacked));
    QCOMPARE(unpacked.version, static_cast<qint16>(5));
    QCOMPARE(unpacked.objectFormat, static_cast<qint16>(2));
    QCOMPARE(unpacked.scripts.size(), 1);
    QCOMPARE(unpacked.scripts[0].scriptName, QStringLiteral("MyCustomScript"));
    QCOMPARE(unpacked.scripts[0].properties.size(), 7);

    // Check individual properties
    QCOMPARE(unpacked.scripts[0].properties[0].name, QStringLiteral("targetRef"));
    QCOMPARE(unpacked.scripts[0].properties[0].objectVal.formId, quint32(0x00012345));
    QCOMPARE(unpacked.scripts[0].properties[0].objectVal.alias, quint16(1));

    QCOMPARE(unpacked.scripts[0].properties[1].name, QStringLiteral("count"));
    QCOMPARE(unpacked.scripts[0].properties[1].intVal, 42);

    QCOMPARE(unpacked.scripts[0].properties[2].name, QStringLiteral("speed"));
    QVERIFY(qAbs(unpacked.scripts[0].properties[2].floatVal - 3.14159f) < 0.0001f);

    QCOMPARE(unpacked.scripts[0].properties[3].name, QStringLiteral("isActive"));
    QCOMPARE(unpacked.scripts[0].properties[3].boolVal, true);

    QCOMPARE(unpacked.scripts[0].properties[4].name, QStringLiteral("label"));
    QCOMPARE(unpacked.scripts[0].properties[4].stringVal, QStringLiteral("Hello Skyrim"));

    QCOMPARE(unpacked.scripts[0].properties[5].name, QStringLiteral("items"));
    QCOMPARE(unpacked.scripts[0].properties[5].arrayObject.size(), 2);
    QCOMPARE(unpacked.scripts[0].properties[5].arrayObject[1].formId, quint32(0x000B2222));

    QCOMPARE(unpacked.scripts[0].properties[6].name, QStringLiteral("levels"));
    QCOMPARE(unpacked.scripts[0].properties[6].arrayInt.size(), 5);
    QCOMPARE(unpacked.scripts[0].properties[6].arrayInt[4], 50);

    QVERIFY(unpacked == vmad);

    // Re-pack must be byte-identical
    const QByteArray repacked = VmadData::pack(unpacked);
    QCOMPARE(repacked, packed);
}

void TestVmad::testQuestRecordVmadRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString filePath = dir.filePath(QStringLiteral("test_quest.esp"));

    QuestRecord original;
    original.blank();
    original.initComponents();
    original.editorId = QStringLiteral("MyScriptedQuest");
    original.formId = 0x01000800;
    original.questName = QStringLiteral("The Ancient Artifact");

    original.hasVmad = true;
    original.vmad.version = 5;
    original.vmad.objectFormat = 2;

    AttachedScript script;
    script.scriptName = QStringLiteral("QF_MyScriptedQuest_01000800");
    ScriptProperty p;
    p.name = QStringLiteral("RewardGold");
    p.type = ScriptProperty::Int;
    p.intVal = 250;
    script.properties.append(p);
    original.vmad.scripts.append(script);

    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        ESMWriter writer;
        writer.setAuthor("Test");
        writer.save(file);
        RecHeader recHeader;
        recHeader.flags.val = 0;
        recHeader.id = original.formId;
        writer.startRecord('QUST', recHeader);
        original.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(filePath);
        reader.open();
        quint32 type = reader.readName();
        QCOMPARE(type, static_cast<quint32>('QUST'));
        QuestRecord loaded;
        loaded.load(reader, false);
        QCOMPARE(loaded.formId, original.formId);
        QVERIFY(loaded.hasVmad);
        QCOMPARE(loaded.vmad.scripts.size(), 1);
        QCOMPARE(loaded.vmad.scripts[0].scriptName, QStringLiteral("QF_MyScriptedQuest_01000800"));
        QVERIFY(loaded.vmad == original.vmad);
        QCOMPARE(loaded.vmad.scripts[0].properties[0].name, QStringLiteral("RewardGold"));
        QCOMPARE(loaded.vmad.scripts[0].properties[0].intVal, 250);
    }
}

void TestVmad::testInfoRecordVmadRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString filePath = dir.filePath(QStringLiteral("test_info.esp"));

    InfoRecord original;
    original.blank();
    original.initComponents();
    original.editorId = QStringLiteral("MyTopicInfo_01");
    original.formId = 0x01000801;
    original.responseText = QStringLiteral("I have a task for you.");

    original.hasVmad = true;
    original.vmad.version = 5;
    original.vmad.objectFormat = 2;

    AttachedScript script;
    script.scriptName = QStringLiteral("TIF__01000801");
    ScriptProperty p;
    p.name = QStringLiteral("QuestRef");
    p.type = ScriptProperty::Object;
    p.objectVal.formId = 0x01000800;
    script.properties.append(p);
    original.vmad.scripts.append(script);

    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        ESMWriter writer;
        writer.setAuthor("Test");
        writer.save(file);
        RecHeader recHeader;
        recHeader.flags.val = 0;
        recHeader.id = original.formId;
        writer.startRecord('INFO', recHeader);
        original.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(filePath);
        reader.open();
        quint32 type = reader.readName();
        QCOMPARE(type, static_cast<quint32>('INFO'));
        InfoRecord loaded;
        loaded.load(reader, false);

        QCOMPARE(loaded.formId, original.formId);
        QVERIFY(loaded.hasVmad);
        QCOMPARE(loaded.vmad.scripts.size(), 1);
        QCOMPARE(loaded.vmad.scripts[0].scriptName, QStringLiteral("TIF__01000801"));
        QCOMPARE(loaded.vmad.scripts[0].properties.size(), 1);
        QCOMPARE(loaded.vmad.scripts[0].properties[0].name, QStringLiteral("QuestRef"));
        QCOMPARE(loaded.vmad.scripts[0].properties[0].objectVal.formId, quint32(0x01000800));
        QVERIFY(loaded.vmad == original.vmad);
    }
}

void TestVmad::testRealSkyrimVmadBytes()
{
    // Real VMAD payload extracted from Skyrim.esm RiftenEvidenceChestKey (80 bytes)
    const QByteArray rawBytes = QByteArray::fromHex(
        "050002000100220064656661756c7473657473746167656f6e706c6179657261637175697265"
        "6974656d00020005006d7951535401010000ffffd86803000a005374616765546f5365740301"
        "14000000"
    );

    VmadData vmad;
    QVERIFY(VmadData::unpack(rawBytes, vmad));
    QCOMPARE(vmad.version, static_cast<qint16>(5));
    QCOMPARE(vmad.objectFormat, static_cast<qint16>(2));
    QCOMPARE(vmad.scripts.size(), 1);
    QCOMPARE(vmad.scripts[0].scriptName, QStringLiteral("defaultsetstageonplayeracquireitem"));
    QCOMPARE(vmad.scripts[0].properties.size(), 2);

    QCOMPARE(vmad.scripts[0].properties[0].name, QStringLiteral("myQST"));
    QCOMPARE(vmad.scripts[0].properties[0].type, ScriptProperty::Object);
    QCOMPARE(vmad.scripts[0].properties[0].objectVal.formId, quint32(0x000368D8));

    QCOMPARE(vmad.scripts[0].properties[1].name, QStringLiteral("StageToSet"));
    QCOMPARE(vmad.scripts[0].properties[1].type, ScriptProperty::Int);
    QCOMPARE(vmad.scripts[0].properties[1].intVal, 20);

    const QByteArray repacked = VmadData::pack(vmad);
    QCOMPARE(repacked, rawBytes);
}

QTEST_MAIN(TestVmad)
#include "test_vmad.moc"
