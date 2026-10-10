#include <QtTest>
#include <QTemporaryDir>

#include "../../src/model/world/idcollection.hpp"
#include "../../libs/files/esm/weaprecord.hpp"
#include "../../libs/files/esm/armorrecord.hpp"
#include "../../libs/files/esm/statrecord.hpp"
#include "../../libs/files/esm/treerecord.hpp"
#include "../../libs/files/esm/actirecord.hpp"
#include "../../libs/files/esm/miscrecord.hpp"
#include "../../libs/files/esm/bookrecord.hpp"
#include "../../libs/files/esm/ingrrecord.hpp"
#include "../../libs/files/esm/alchrecord.hpp"
#include "../../libs/files/esm/contrecord.hpp"
#include "../../libs/files/esm/refrecord.hpp"
#include "../../src/model/world/data.hpp"
#include "../../src/model/window/objectwindow.hpp"
#include "../../src/view/window/useinfodialog.hpp"
class TestObjectWindowModelPath : public QObject
{
    Q_OBJECT

private slots:
    void testWeaponModelPath();
    void testArmorModelPath();
    void testStatModelPath();
    void testTreeModelPath();
    void testActiModelPath();
    void testMiscModelPath();
    void testBookModelPath();
    void testIngrModelPath();
    void testAlchModelPath();
    void testContModelPath();
    void testEmptyCollection();
    void testHierarchyGroupNodes();
    void testFilterWildcardAndReset();
    void testUseInfoDialog();
};
void TestObjectWindowModelPath::testWeaponModelPath()
{
    IdCollection<WeaponRecord> collection;
    
    WeaponRecord weap;
    weap.editorId = "TestWeapon";
    weap.formId = 0x00000001;
    weap.modelPath = "meshes\\testweapon.nif";
    collection.add(weap);
    
    QCOMPARE(collection.size(), 1);
    QCOMPARE(collection.getRecord(0).get().modelPath, QString("meshes\\testweapon.nif"));
}

void TestObjectWindowModelPath::testArmorModelPath()
{
    IdCollection<ArmorRecord> collection;
    
    ArmorRecord armor;
    armor.editorId = "TestArmor";
    armor.formId = 0x00000001;
    armor.modelPath = "meshes\\testarmor.nif";
    collection.add(armor);
    
    QCOMPARE(collection.size(), 1);
    QCOMPARE(collection.getRecord(0).get().modelPath, QString("meshes\\testarmor.nif"));
}

void TestObjectWindowModelPath::testStatModelPath()
{
    IdCollection<StatRecord> collection;
    
    StatRecord stat;
    stat.editorId = "TestStatic";
    stat.formId = 0x00000001;
    stat.modelPath = "meshes\\teststatic.nif";
    collection.add(stat);
    
    QCOMPARE(collection.size(), 1);
    QCOMPARE(collection.getRecord(0).get().modelPath, QString("meshes\\teststatic.nif"));
}

void TestObjectWindowModelPath::testTreeModelPath()
{
    IdCollection<TreeRecord> collection;
    
    TreeRecord tree;
    tree.editorId = "TestTree";
    tree.formId = 0x00000001;
    tree.modelPath = "meshes\\testtree.nif";
    collection.add(tree);
    
    QCOMPARE(collection.size(), 1);
    QCOMPARE(collection.getRecord(0).get().modelPath, QString("meshes\\testtree.nif"));
}

void TestObjectWindowModelPath::testActiModelPath()
{
    IdCollection<ActiRecord> collection;
    
    ActiRecord acti;
    acti.editorId = "TestActivator";
    acti.formId = 0x00000001;
    acti.modelPath = "meshes\\testactivator.nif";
    collection.add(acti);
    
    QCOMPARE(collection.size(), 1);
    QCOMPARE(collection.getRecord(0).get().modelPath, QString("meshes\\testactivator.nif"));
}

void TestObjectWindowModelPath::testMiscModelPath()
{
    IdCollection<MiscRecord> collection;
    
    MiscRecord misc;
    misc.editorId = "TestMisc";
    misc.formId = 0x00000001;
    misc.modelPath = "meshes\\testmisc.nif";
    collection.add(misc);
    
    QCOMPARE(collection.size(), 1);
    QCOMPARE(collection.getRecord(0).get().modelPath, QString("meshes\\testmisc.nif"));
}

void TestObjectWindowModelPath::testBookModelPath()
{
    IdCollection<BookRecord> collection;
    
    BookRecord book;
    book.editorId = "TestBook";
    book.formId = 0x00000001;
    book.modelPath = "meshes\\testbook.nif";
    collection.add(book);
    
    QCOMPARE(collection.size(), 1);
    QCOMPARE(collection.getRecord(0).get().modelPath, QString("meshes\\testbook.nif"));
}

void TestObjectWindowModelPath::testIngrModelPath()
{
    IdCollection<IngrRecord> collection;
    
    IngrRecord ingr;
    ingr.editorId = "TestIngredient";
    ingr.formId = 0x00000001;
    ingr.modelPath = "meshes\\testingredient.nif";
    collection.add(ingr);
    
    QCOMPARE(collection.size(), 1);
    QCOMPARE(collection.getRecord(0).get().modelPath, QString("meshes\\testingredient.nif"));
}

void TestObjectWindowModelPath::testAlchModelPath()
{
    IdCollection<AlchRecord> collection;
    
    AlchRecord alch;
    alch.editorId = "TestPotion";
    alch.formId = 0x00000001;
    alch.modelPath = "meshes\\testpotion.nif";
    collection.add(alch);
    
    QCOMPARE(collection.size(), 1);
    QCOMPARE(collection.getRecord(0).get().modelPath, QString("meshes\\testpotion.nif"));
}

void TestObjectWindowModelPath::testContModelPath()
{
    IdCollection<ContRecord> collection;
    
    ContRecord cont;
    cont.editorId = "TestContainer";
    cont.formId = 0x00000001;
    cont.modelPath = "meshes\\testcontainer.nif";
    collection.add(cont);
    
    QCOMPARE(collection.size(), 1);
    QCOMPARE(collection.getRecord(0).get().modelPath, QString("meshes\\testcontainer.nif"));
}

void TestObjectWindowModelPath::testEmptyCollection()
{
    IdCollection<WeaponRecord> collection;
    QCOMPARE(collection.size(), 0);
}

void TestObjectWindowModelPath::testHierarchyGroupNodes()
{
    const FilePaths paths;
    Data data(QStringList(), paths);
    ObjectWindowModel model;
    model.setData(&data);

    // Level 0: Groups
    int groupCount = model.rowCount(QModelIndex());
    QVERIFY(groupCount >= 8);

    QStringList expectedGroups = {
        "All Forms", "Actors", "Items", "Magic", "World Objects",
        "Gameplay", "Audio", "Dialogue", "Special"
    };

    QStringList actualGroups;
    for (int r = 0; r < groupCount; ++r) {
        QModelIndex idx = model.index(r, 0, QModelIndex());
        actualGroups.append(model.data(idx, Qt::DisplayRole).toString());
    }

    for (const QString& g : expectedGroups) {
        QVERIFY2(actualGroups.contains(g), qPrintable(QString("Missing expected group: %1").arg(g)));
    }

    // Level 1: Categories inside "Actors"
    int actorsRow = actualGroups.indexOf("Actors");
    QVERIFY(actorsRow >= 0);
    QModelIndex actorsIdx = model.index(actorsRow, 0, QModelIndex());
    int actorsChildCount = model.rowCount(actorsIdx);
    QVERIFY(actorsChildCount > 0);

    QStringList actorCats;
    for (int r = 0; r < actorsChildCount; ++r) {
        QModelIndex catIdx = model.index(r, 0, actorsIdx);
        actorCats.append(model.data(catIdx, Qt::DisplayRole).toString());
    }
    QVERIFY(actorCats.contains("NPC"));
    QVERIFY(actorCats.contains("Creature"));

    // Level 1: Categories inside "Magic"
    int magicRow = actualGroups.indexOf("Magic");
    QVERIFY(magicRow >= 0);
    QModelIndex magicIdx = model.index(magicRow, 0, QModelIndex());
    int magicChildCount = model.rowCount(magicIdx);
    QVERIFY(magicChildCount > 0);

    QStringList magicCats;
    for (int r = 0; r < magicChildCount; ++r) {
        QModelIndex catIdx = model.index(r, 0, magicIdx);
        magicCats.append(model.data(catIdx, Qt::DisplayRole).toString());
    }
    QVERIFY(magicCats.contains("Spell"));
    QVERIFY(magicCats.contains("Magic Effect"));
}

void TestObjectWindowModelPath::testFilterWildcardAndReset()
{
    const FilePaths paths;
    Data data(QStringList(), paths);
    WeaponRecord w1;
    w1.editorId = "IronSword";
    w1.formId = 0x00010001;
    data.getWeaponCollection().add(w1);

    WeaponRecord w2;
    w2.editorId = "SteelDagger";
    w2.formId = 0x00010002;
    data.getWeaponCollection().add(w2);

    WeaponRecord w3;
    w3.editorId = "SilverSword";
    w3.formId = 0x00010003;
    data.getWeaponCollection().add(w3);

    ObjectWindowModel model;
    model.setData(&data);

    int catId = -1, recIdx = -1;
    QModelIndex foundIdx;


    // Unfiltered: all 3 can be found
    QVERIFY(model.findRecord("IronSword", catId, recIdx, foundIdx));
    QVERIFY(model.findRecord("SilverSword", catId, recIdx, foundIdx));

    // Wildcard filter: *Sword*
    model.applyFilter("*Sword*");
    QVERIFY(model.findRecord("IronSword", catId, recIdx, foundIdx));
    QVERIFY(model.findRecord("SilverSword", catId, recIdx, foundIdx));
    QVERIFY(!model.findRecord("SteelDagger", catId, recIdx, foundIdx));

    // Clear filter: restores all records without loss
    model.applyFilter("");
    QVERIFY(model.findRecord("IronSword", catId, recIdx, foundIdx));
    QVERIFY(model.findRecord("SteelDagger", catId, recIdx, foundIdx));
    QVERIFY(model.findRecord("SilverSword", catId, recIdx, foundIdx));
}

void TestObjectWindowModelPath::testUseInfoDialog()
{
    const FilePaths paths;
    Data data(QStringList(), paths);
    StatRecord stat;
    stat.editorId = "DungeonDoorArch";
    stat.formId = 0x00020001;
    data.getStatCollection().add(stat);

    // Place two references of this static
    RefrRecord ref1;
    ref1.formId = 0x00030001;
    ref1.editorId = "ArchRef01";
    ref1.baseId = 0x00020001;
    ref1.posX = 100.0f;
    ref1.posY = 200.0f;
    ref1.posZ = 300.0f;
    data.getRefrCollection().add(ref1);

    RefrRecord ref2;
    ref2.formId = 0x00030002;
    ref2.editorId = "ArchRef02";
    ref2.baseId = 0x00020001;
    ref2.posX = 500.0f;
    ref2.posY = 600.0f;
    ref2.posZ = 700.0f;
    data.getRefrCollection().add(ref2);

    UseInfoDialog dlg(&data, stat.formId, stat.editorId);
    QCOMPARE(dlg.totalUsesCount(), 2);
}

#include "test_objectwindow.moc"
QTEST_MAIN(TestObjectWindowModelPath)
