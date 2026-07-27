#include <QtTest>

#include "../../libs/components/tier3_components.hpp"
#include "../../libs/files/esm/common.hpp"

using namespace tescomponents;

class TestTier3Components : public QObject
{
    Q_OBJECT

private slots:
    void testTESFlags();
    void testBGSSoundDescriptor();
    void testTESWeatherData();
    void testBGSRefData();
    void testTESActorBaseData();
    void testTESSpellList();
    void testTESAIForm();
    void testTESBodyParts();
    void testTESSkills();
    void testTESAttributes();
    void testTESNPCFaceGen();
    void testNullOther();
};

// ---------------------------------------------------------------------------
// TESFlags_Component
// ---------------------------------------------------------------------------
void TestTier3Components::testTESFlags()
{
    TESFlags_Component a;
    a.flags = 0x42;

    auto cloned = a.clone();
    QVERIFY(cloned->isEqualTo(&a));
    QCOMPARE(cloned->className(), QStringLiteral("TESFlags"));

    TESFlags_Component b;
    b.copyFrom(&a);
    QCOMPARE(b.flags, quint32(0x42));
    QVERIFY(b.isEqualTo(&a));

    TESFlags_Component c;
    c.flags = 0x10;
    QVERIFY(!c.isEqualTo(&a));

    c.mergeWith(&a);
    QCOMPARE(c.flags, quint32(0x42));

    TESFlags_Component f;
    QVERIFY(f.canHandle(NAME('FNAM')));
    QVERIFY(f.canHandle(NAME('FLAG')));
    QVERIFY(!f.canHandle(NAME('MODL')));

    QVERIFY(!f.isEqualTo(nullptr));
    TESFlags_Component other;
    other.copyFrom(nullptr);
    QCOMPARE(other.flags, quint32(0));
}

// ---------------------------------------------------------------------------
// BGSSoundDescriptor_Component
// ---------------------------------------------------------------------------
void TestTier3Components::testBGSSoundDescriptor()
{
    BGSSoundDescriptor_Component a;
    a.soundFile = QStringLiteral("sound/test.wav");
    a.soundFlags = 0x05;

    auto cloned = a.clone();
    QVERIFY(cloned->isEqualTo(&a));
    QCOMPARE(cloned->className(), QStringLiteral("BGSSoundDescriptor"));

    BGSSoundDescriptor_Component b;
    b.copyFrom(&a);
    QCOMPARE(b.soundFile, QStringLiteral("sound/test.wav"));
    QCOMPARE(b.soundFlags, quint32(0x05));
    QVERIFY(b.isEqualTo(&a));

    BGSSoundDescriptor_Component c;
    c.soundFile = QStringLiteral("other.wav");
    QVERIFY(!c.isEqualTo(&a));

    c.mergeWith(&a);
    QCOMPARE(c.soundFile, QStringLiteral("sound/test.wav"));

    BGSSoundDescriptor_Component f;
    QVERIFY(f.canHandle(NAME('FNAM')));
    QVERIFY(f.canHandle(NAME('SNDD')));
    QVERIFY(f.canHandle(NAME('SNDX')));
    QVERIFY(!f.canHandle(NAME('MODL')));
}

// ---------------------------------------------------------------------------
// TESWeatherData_Component
// ---------------------------------------------------------------------------
void TestTier3Components::testTESWeatherData()
{
    TESWeatherData_Component a;
    a.sunTexture = QStringLiteral("weather/sun.dds");
    a.weatherFlags = 0x11;

    auto cloned = a.clone();
    QVERIFY(cloned->isEqualTo(&a));
    QCOMPARE(cloned->className(), QStringLiteral("TESWeatherData"));

    TESWeatherData_Component b;
    b.copyFrom(&a);
    QCOMPARE(b.sunTexture, QStringLiteral("weather/sun.dds"));
    QCOMPARE(b.weatherFlags, quint32(0x11));
    QVERIFY(b.isEqualTo(&a));

    TESWeatherData_Component c;
    c.weatherFlags = 0xFF;
    QVERIFY(!c.isEqualTo(&a));

    c.mergeWith(&a);
    QCOMPARE(c.weatherFlags, quint32(0x11));

    TESWeatherData_Component f;
    QVERIFY(f.canHandle(NAME('SNAM')));
    QVERIFY(f.canHandle(NAME('FNAM')));
    QVERIFY(f.canHandle(NAME('FLAG')));
    QVERIFY(!f.canHandle(NAME('MODL')));
}

// ---------------------------------------------------------------------------
// BGSRefData_Component
// ---------------------------------------------------------------------------
void TestTier3Components::testBGSRefData()
{
    BGSRefData_Component a;
    a.baseId = 0xABCD;
    a.posX = 1.5f; a.posY = 2.5f; a.posZ = 3.5f;
    a.rotX = 0.1f; a.rotY = 0.2f; a.rotZ = 0.3f;
    a.scale = 2.0f;
    a.owner = 0x1234;
    a.lockLevel = 50;
    a.initiallyDisabled = true;
    a.scriptIds = {1, 2, 3};

    auto cloned = a.clone();
    QVERIFY(cloned->isEqualTo(&a));
    QCOMPARE(cloned->className(), QStringLiteral("BGSRefData"));

    BGSRefData_Component b;
    b.copyFrom(&a);
    QCOMPARE(b.baseId, quint32(0xABCD));
    QCOMPARE(b.posX, 1.5f);
    QCOMPARE(b.scale, 2.0f);
    QCOMPARE(b.owner, quint32(0x1234));
    QCOMPARE(b.lockLevel, quint32(50));
    QVERIFY(b.initiallyDisabled);
    QVector<quint32> expectedScripts{1, 2, 3};
    QCOMPARE(b.scriptIds, expectedScripts);
    QVERIFY(b.isEqualTo(&a));

    BGSRefData_Component c;
    c.scale = 5.0f;
    QVERIFY(!c.isEqualTo(&a));

    c.mergeWith(&a);
    QCOMPARE(c.scale, 2.0f);

    BGSRefData_Component f;
    QVERIFY(f.canHandle(NAME('NAME')));
    QVERIFY(f.canHandle(NAME('DATA')));
    QVERIFY(f.canHandle(NAME('XOWN')));
    QVERIFY(f.canHandle(NAME('DNAM')));
    QVERIFY(f.canHandle(NAME('XESP')));
    QVERIFY(f.canHandle(NAME('SCRI')));
    QVERIFY(!f.canHandle(NAME('MODL')));
}

// ---------------------------------------------------------------------------
// TESActorBaseData_Component
// ---------------------------------------------------------------------------
void TestTier3Components::testTESActorBaseData()
{
    TESActorBaseData_Component a;
    a.flags = 0x100;
    a.baseSpell = 100;
    a.fatigue = 200;
    a.barterGold = 50;
    a.level = 10;
    a.calcMin = 1;
    a.calcMax = 99;
    a.speedMult = 100;

    auto cloned = a.clone();
    QVERIFY(cloned->isEqualTo(&a));
    QCOMPARE(cloned->className(), QStringLiteral("TESActorBaseData"));

    TESActorBaseData_Component b;
    b.copyFrom(&a);
    QCOMPARE(b.flags, quint32(0x100));
    QCOMPARE(b.baseSpell, quint16(100));
    QCOMPARE(b.fatigue, quint16(200));
    QCOMPARE(b.barterGold, quint16(50));
    QCOMPARE(b.level, qint16(10));
    QCOMPARE(b.calcMin, quint16(1));
    QCOMPARE(b.calcMax, quint16(99));
    QCOMPARE(b.speedMult, quint16(100));
    QVERIFY(b.isEqualTo(&a));

    TESActorBaseData_Component c;
    c.level = 5;
    QVERIFY(!c.isEqualTo(&a));

    c.mergeWith(&a);
    QCOMPARE(c.level, qint16(10));

    TESActorBaseData_Component f;
    QVERIFY(f.canHandle(NAME('ACBS')));
    QVERIFY(!f.canHandle(NAME('MODL')));
}

// ---------------------------------------------------------------------------
// TESSpellList_Component
// ---------------------------------------------------------------------------
void TestTier3Components::testTESSpellList()
{
    TESSpellList_Component a;
    a.spells = {0x10, 0x20, 0x30};

    auto cloned = a.clone();
    QVERIFY(cloned->isEqualTo(&a));
    QCOMPARE(cloned->className(), QStringLiteral("TESSpellList"));

    TESSpellList_Component b;
    b.copyFrom(&a);
    QVector<quint32> expectedSpells{0x10, 0x20, 0x30};
    QCOMPARE(b.spells, expectedSpells);
    QVERIFY(b.isEqualTo(&a));

    TESSpellList_Component c;
    c.spells = {0x10};
    QVERIFY(!c.isEqualTo(&a));

    c.mergeWith(&a);
    QCOMPARE(c.spells, a.spells);

    TESSpellList_Component f;
    QVERIFY(f.canHandle(NAME('SPLO')));
    QVERIFY(!f.canHandle(NAME('MODL')));
}

// ---------------------------------------------------------------------------
// TESAIForm_Component
// ---------------------------------------------------------------------------
void TestTier3Components::testTESAIForm()
{
    TESAIForm_Component a;
    a.aggression = 3;
    a.confidence = 4;
    a.energy = 50;
    a.morality = 2;
    a.mood = -5;
    a.moodSpeed = 10;
    a.disposition = 75;
    a.aggressionLevel = 1;

    auto cloned = a.clone();
    QVERIFY(cloned->isEqualTo(&a));
    QCOMPARE(cloned->className(), QStringLiteral("TESAIForm"));

    TESAIForm_Component b;
    b.copyFrom(&a);
    QCOMPARE(b.aggression, quint8(3));
    QCOMPARE(b.confidence, quint8(4));
    QCOMPARE(b.energy, quint8(50));
    QCOMPARE(b.morality, quint8(2));
    QCOMPARE(b.mood, qint16(-5));
    QCOMPARE(b.moodSpeed, quint8(10));
    QCOMPARE(b.disposition, quint8(75));
    QCOMPARE(b.aggressionLevel, quint8(1));
    QVERIFY(b.isEqualTo(&a));

    TESAIForm_Component c;
    c.aggression = 1;
    QVERIFY(!c.isEqualTo(&a));

    c.mergeWith(&a);
    QCOMPARE(c.aggression, quint8(3));

    TESAIForm_Component f;
    QVERIFY(f.canHandle(NAME('AIDT')));
    QVERIFY(!f.canHandle(NAME('MODL')));
}

// ---------------------------------------------------------------------------
// TESBodyParts_Component
// ---------------------------------------------------------------------------
void TestTier3Components::testTESBodyParts()
{
    TESBodyParts_Component a;
    a.partType = 1;
    a.flags = 0x22;
    a.partCount = 4;

    auto cloned = a.clone();
    QVERIFY(cloned->isEqualTo(&a));
    QCOMPARE(cloned->className(), QStringLiteral("TESBodyParts"));

    TESBodyParts_Component b;
    b.copyFrom(&a);
    QCOMPARE(b.partType, quint32(1));
    QCOMPARE(b.flags, quint32(0x22));
    QCOMPARE(b.partCount, quint32(4));
    QVERIFY(b.isEqualTo(&a));

    TESBodyParts_Component c;
    c.partCount = 9;
    QVERIFY(!c.isEqualTo(&a));

    c.mergeWith(&a);
    QCOMPARE(c.partCount, quint32(4));

    TESBodyParts_Component f;
    QVERIFY(f.canHandle(NAME('BODT')));
    QVERIFY(f.canHandle(NAME('BOD2')));
    QVERIFY(!f.canHandle(NAME('MODL')));
}

// ---------------------------------------------------------------------------
// TESSkills_Component
// ---------------------------------------------------------------------------
void TestTier3Components::testTESSkills()
{
    TESSkills_Component a;
    a.skillValues = {5, 10, 15, 20};

    auto cloned = a.clone();
    QVERIFY(cloned->isEqualTo(&a));
    QCOMPARE(cloned->className(), QStringLiteral("TESSkills"));

    TESSkills_Component b;
    b.copyFrom(&a);
    QVector<qint32> expectedSkills{5, 10, 15, 20};
    QCOMPARE(b.skillValues, expectedSkills);
    QVERIFY(b.isEqualTo(&a));

    TESSkills_Component c;
    c.skillValues = {1, 2};
    QVERIFY(!c.isEqualTo(&a));

    c.mergeWith(&a);
    QCOMPARE(c.skillValues, a.skillValues);

    TESSkills_Component f;
    QVERIFY(f.canHandle(NAME('SKIL')));
    QVERIFY(!f.canHandle(NAME('MODL')));
}

// ---------------------------------------------------------------------------
// TESAttributes_Component
// ---------------------------------------------------------------------------
void TestTier3Components::testTESAttributes()
{
    TESAttributes_Component a;
    a.attributes = {10, 20, 30, 40, 50, 60, 70, 80};

    auto cloned = a.clone();
    QVERIFY(cloned->isEqualTo(&a));
    QCOMPARE(cloned->className(), QStringLiteral("TESAttributes"));

    TESAttributes_Component b;
    b.copyFrom(&a);
    QVector<qint32> expectedAttrs{10, 20, 30, 40, 50, 60, 70, 80};
    QCOMPARE(b.attributes, expectedAttrs);
    QVERIFY(b.isEqualTo(&a));

    TESAttributes_Component c;
    c.attributes = {1};
    QVERIFY(!c.isEqualTo(&a));

    c.mergeWith(&a);
    QCOMPARE(c.attributes, a.attributes);

    TESAttributes_Component f;
    QVERIFY(f.canHandle(NAME('ATTR')));
    QVERIFY(f.canHandle(NAME('BYDT')));
    QVERIFY(!f.canHandle(NAME('MODL')));
}

// ---------------------------------------------------------------------------
// TESNPCFaceGen_Component
// ---------------------------------------------------------------------------
void TestTier3Components::testTESNPCFaceGen()
{
    TESNPCFaceGen_Component a;
    a.hairFormId = 0xAAA;
    a.eyesFormId = 0xBBB;
    a.faceTextureFormId = 0xCCC;
    a.headParts = {0x1, 0x2};
    a.faceMorphSym = {0.5f, 1.0f};
    a.faceMorphAsym = {0.25f};

    auto cloned = a.clone();
    QVERIFY(cloned->isEqualTo(&a));
    QCOMPARE(cloned->className(), QStringLiteral("TESNPCFaceGen"));

    TESNPCFaceGen_Component b;
    b.copyFrom(&a);
    QCOMPARE(b.hairFormId, quint32(0xAAA));
    QCOMPARE(b.eyesFormId, quint32(0xBBB));
    QCOMPARE(b.faceTextureFormId, quint32(0xCCC));
    QVector<quint32> expectedHeadParts{0x1, 0x2};
    QVector<float> expectedSym{0.5f, 1.0f};
    QVector<float> expectedAsym{0.25f};
    QCOMPARE(b.headParts, expectedHeadParts);
    QCOMPARE(b.faceMorphSym, expectedSym);
    QCOMPARE(b.faceMorphAsym, expectedAsym);
    QVERIFY(b.isEqualTo(&a));

    TESNPCFaceGen_Component c;
    c.hairFormId = 0x111;
    QVERIFY(!c.isEqualTo(&a));

    c.mergeWith(&a);
    QCOMPARE(c.hairFormId, quint32(0xAAA));

    TESNPCFaceGen_Component f;
    QVERIFY(f.canHandle(NAME('HNAM')));
    QVERIFY(f.canHandle(NAME('ENAM')));
    QVERIFY(f.canHandle(NAME('QNAM')));
    QVERIFY(f.canHandle(NAME('PNAM')));
    QVERIFY(f.canHandle(NAME('NAMA')));
    QVERIFY(f.canHandle(NAME('NAM9')));
    QVERIFY(f.canHandle(NAME('FGGS')));
    QVERIFY(f.canHandle(NAME('FGGA')));
    QVERIFY(f.canHandle(NAME('FGTR')));
    QVERIFY(f.canHandle(NAME('NIFT')));
    QVERIFY(!f.canHandle(NAME('MODL')));
}

// ---------------------------------------------------------------------------
// isEqualTo / copyFrom with null or wrong-class component
// ---------------------------------------------------------------------------
void TestTier3Components::testNullOther()
{
    TESFlags_Component a;
    a.flags = 1;
    QVERIFY(!a.isEqualTo(nullptr));

    TESFlags_Component b;
    b.copyFrom(nullptr);
    QCOMPARE(b.flags, quint32(0));

    // Cross-class isEqualTo should return false
    TESFlags_Component flags;
    BGSSoundDescriptor_Component sound;
    QVERIFY(!flags.isEqualTo(&sound));
    flags.copyFrom(&sound);
    QCOMPARE(flags.flags, quint32(0));
}

#include "test_tier3_components.moc"
QTEST_MAIN(TestTier3Components)