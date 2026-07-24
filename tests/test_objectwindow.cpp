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

#include "test_objectwindow.moc"
QTEST_MAIN(TestObjectWindowModelPath)
