#include <QtTest>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QDebug>

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
#include "../../libs/files/esm/doorrecord.hpp"
#include "../../libs/files/esm/furnrecord.hpp"
#include "../../src/view/window/objectwindowdialog.hpp"
#include <QTableView>
#include <QSplitter>
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
    void testNonStatRecordsExposeDataAndFilter();
    void testHeaderData();
    void testSplitPaneDialog();
    void testModelFolderNormalization();
    void testFolderTreeFiltersTable();
    void test50kFolderBuildPerformance();
    void testTableSorting();
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

void TestObjectWindowModelPath::testNonStatRecordsExposeDataAndFilter()
{
    const FilePaths paths;
    Data data(QStringList(), paths);

    DoorRecord door;
    door.editorId = "DungeonIronDoor";
    door.formId = 0x00040001;
    door.modelPath = "meshes/doors/irondoor01.nif";
    data.getDoorCollection().add(door);

    FurnRecord furn;
    furn.editorId = "LuxuryChair";
    furn.formId = 0x00040002;
    furn.modelPath = "meshes/furniture/chair01.nif";
    data.getFurnCollection().add(furn);

    ObjectWindowModel model;
    model.setData(&data);

    int catId = -1, recIdx = -1;
    QModelIndex foundIdx;

    // Both non-STAT records must be present
    QVERIFY(model.findRecord("DungeonIronDoor", catId, recIdx, foundIdx));
    QCOMPARE(model.getRecordEditorId(catId, recIdx), QString("DungeonIronDoor"));
    QCOMPARE(model.getRecordFormId(catId, recIdx), QString("0x00040001"));
    QCOMPARE(model.getRecordModelPath(catId, recIdx), QString("meshes/doors/irondoor01.nif"));

    QVERIFY(model.findRecord("LuxuryChair", catId, recIdx, foundIdx));
    QCOMPARE(model.getRecordEditorId(catId, recIdx), QString("LuxuryChair"));
    QCOMPARE(model.getRecordFormId(catId, recIdx), QString("0x00040002"));
    QCOMPARE(model.getRecordModelPath(catId, recIdx), QString("meshes/furniture/chair01.nif"));

    // Applying a filter must NOT blank out non-STAT records
    model.applyFilter("*Door*");
    QVERIFY(model.findRecord("DungeonIronDoor", catId, recIdx, foundIdx));
    QCOMPARE(model.getRecordEditorId(catId, recIdx), QString("DungeonIronDoor"));
    QCOMPARE(model.getRecordFormId(catId, recIdx), QString("0x00040001"));
    QVERIFY(!model.findRecord("LuxuryChair", catId, recIdx, foundIdx));

    // Resetting filter restores both
    model.applyFilter("");
    QVERIFY(model.findRecord("DungeonIronDoor", catId, recIdx, foundIdx));
    QCOMPARE(model.getRecordEditorId(catId, recIdx), QString("DungeonIronDoor"));
    QVERIFY(model.findRecord("LuxuryChair", catId, recIdx, foundIdx));
    QCOMPARE(model.getRecordEditorId(catId, recIdx), QString("LuxuryChair"));
}

void TestObjectWindowModelPath::testHeaderData()
{
    const FilePaths paths;
    Data data(QStringList(), paths);
    ObjectWindowModel model;
    model.setData(&data);

    QCOMPARE(model.columnCount(), 4);
    QCOMPARE(model.headerData(0, Qt::Horizontal).toString(), QString("Editor ID"));
    QCOMPARE(model.headerData(1, Qt::Horizontal).toString(), QString("Form ID"));
    QCOMPARE(model.headerData(2, Qt::Horizontal).toString(), QString("Type"));
    QCOMPARE(model.headerData(3, Qt::Horizontal).toString(), QString("Model"));
}

void TestObjectWindowModelPath::testSplitPaneDialog()
{
    const FilePaths paths;
    Data data(QStringList(), paths);

    DoorRecord door;
    door.editorId = "TestVaultDoor";
    door.formId = 0x00050001;
    door.modelPath = "meshes/doors/vaultdoor01.nif";
    data.getDoorCollection().add(door);

    ObjectWindowDialog dlg(&data);
    QVERIFY(dlg.getTreeView() != nullptr);
    QVERIFY(dlg.getRecordTable() != nullptr);

    // selectRecord finds the record across the two-pane model
    QVERIFY(dlg.selectRecord("TestVaultDoor"));
    QVERIFY(dlg.getRecordTable()->currentIndex().isValid());
}

void TestObjectWindowModelPath::testModelFolderNormalization()
{
    QCOMPARE(ObjectWindowModel::modelFolder("Meshes\\Armor\\Iron\\Helm.nif"), QString("armor/iron"));
    QCOMPARE(ObjectWindowModel::modelFolder("Data\\Meshes\\Armor\\x.nif"), QString("armor"));
    QCOMPARE(ObjectWindowModel::modelFolder("armor/x.nif"), QString("armor"));
    QCOMPARE(ObjectWindowModel::modelFolder("meshes/x.nif"), QString());
    QCOMPARE(ObjectWindowModel::modelFolder("x.nif"), QString());
    QCOMPARE(ObjectWindowModel::modelFolder(QString()), QString());
}

void TestObjectWindowModelPath::testFolderTreeFiltersTable()
{
    const FilePaths paths;
    Data data(QStringList(), paths);

    const char* models[] = { "meshes/Doors/Vault/a.nif", "meshes\\doors\\vault\\b.nif",
                             "meshes/doors/wood/c.nif", "d.nif" };
    quint32 formId = 0x00060001;
    for (const char* model : models)
    {
        DoorRecord door;
        door.editorId = QString("Door_%1").arg(formId);
        door.formId = formId++;
        door.modelPath = model;
        data.getDoorCollection().add(door);
    }

    ObjectWindowDialog dlg(&data);
    QVERIFY(dlg.selectRecord("Door_393217"));

    QTreeView* tree = dlg.getTreeView();
    QAbstractItemModel* nav = tree->model();
    const QModelIndex category = tree->currentIndex();
    QVERIFY(category.isValid());

    // (no folder) (1), doors (3) > vault (2), wood (1)
    QCOMPARE(nav->rowCount(category), 2);
    const QModelIndex noFolder = nav->index(0, 0, category);
    QCOMPARE(nav->data(noFolder).toString(), QString("(no folder) (1)"));
    const QModelIndex doors = nav->index(1, 0, category);
    QCOMPARE(nav->data(doors).toString(), QString("Doors (3)"));
    QCOMPARE(nav->rowCount(doors), 2);
    const QModelIndex vault = nav->index(0, 0, doors);
    QCOMPARE(nav->data(vault).toString(), QString("Vault (2)"));

    QAbstractItemModel* table = dlg.getRecordTable()->model();
    QCOMPARE(table->rowCount(), 4);

    tree->setCurrentIndex(vault);
    QCOMPARE(table->rowCount(), 2);
    tree->setCurrentIndex(doors);
    QCOMPARE(table->rowCount(), 3);
    tree->setCurrentIndex(noFolder);
    QCOMPARE(table->rowCount(), 1);

    // A record picked from a filtered table resolves to a real record (edit/clone/delete path).
    tree->setCurrentIndex(vault);
    dlg.getRecordTable()->setCurrentIndex(table->index(0, 0));
    QVERIFY(dlg.currentIndex().isValid());

    tree->setCurrentIndex(category);
    QCOMPARE(table->rowCount(), 4);

    // A partial filter keeps its matches (no anchoring) and the folder tree follows it.
    dlg.getFilterEdit()->setText("Door_39321");
    QCOMPARE(table->rowCount(), 3);
    dlg.getFilterEdit()->clear();
    QCOMPARE(table->rowCount(), 4);

    // Find Next/Previous walk Editor IDs and wrap.
    QSignalSpy selected(&dlg, &ObjectWindowDialog::recordSelected);
    QVERIFY(dlg.findRecordText("393219", true));
    QCOMPARE(selected.last().at(2).toString(), QString("Door_393219"));
    QVERIFY(dlg.findRecordText("Door_", false));
    QCOMPARE(selected.last().at(2).toString(), QString("Door_393218"));
    QVERIFY(dlg.findRecordText("393217", false));
    QVERIFY(dlg.findRecordText("Door_", false)); // wraps from the first record to the last
    QCOMPARE(selected.last().at(2).toString(), QString("Door_393220"));
    QVERIFY(dlg.findRecordText("Door_", true));  // and forward again to the first
    QCOMPARE(selected.last().at(2).toString(), QString("Door_393217"));
    QVERIFY(!dlg.findRecordText("NoSuchEditorId", true));
}


void TestObjectWindowModelPath::test50kFolderBuildPerformance()
{
    const FilePaths paths;
    Data data(QStringList(), paths);

    static const char* kDirs[] = {
        "meshes/architecture/whiterun/houses/wrhouse%1.nif",
        "meshes/architecture/solitude/walls/solwall%1.nif",
        "meshes/dungeons/caves/rocks/rock%1.nif",
        "meshes/clutter/books/tome%1.nif",
        "meshes/weapons/iron/sword%1.nif",
        "meshes/armor/steel/cuirass%1.nif",
        "meshes/landscape/trees/tree%1.nif",
        "meshes/actors/character/hair/hair%1.nif",
        "meshes/root_model%1.nif",
        ""
    };

    auto& coll = data.getStatCollection();
    for (int i = 0; i < 50000; ++i)
    {
        StatRecord stat;
        stat.editorId = QString("Stat_%1").arg(i);
        stat.formId = 0x00100000 + i;
        const char* pattern = kDirs[i % 10];
        if (pattern[0] != '\0')
            stat.modelPath = QString(pattern).arg(i % 100);
        coll.add(stat);
    }

    ObjectWindowModel model;
    model.setData(&data);

    int statCat = -1;
    for (int c = 0; c < model.getCategoryCount(); ++c)
    {
        if (model.getCategoryType(c) == CkId::Type_Stat_)
        {
            statCat = c;
            break;
        }
    }
    QVERIFY(statCat >= 0);
    QCOMPARE(model.getRecordCountForCategory(statCat), 50000);

    QElapsedTimer timer;
    timer.start();
    QVector<ModelFolder> folders = model.getModelFolders(statCat);
    const qint64 elapsedMs = timer.elapsed();

    qInfo() << "50,000 records folder build took:" << elapsedMs << "ms, found" << folders.size() << "folders";

    QVERIFY(!folders.isEmpty());
    QCOMPARE(folders[0].path, ObjectWindowModel::NoFolder);
    QCOMPARE(folders[0].count, 10000);
    QVERIFY(elapsedMs < 500);
}

void TestObjectWindowModelPath::testTableSorting()
{
    const FilePaths paths;
    Data data(QStringList(), paths);
    auto& coll = data.getStatCollection();

    StatRecord s1;
    s1.editorId = "Stat_Z";
    s1.formId = 0x00020000;
    s1.modelPath = "meshes/clutter/vase.nif";
    coll.add(s1);

    StatRecord s2;
    s2.editorId = "Stat_A";
    s2.formId = 0x00010000;
    s2.modelPath = "meshes/clutter/apple.nif";
    coll.add(s2);

    ObjectWindowDialog dlg(&data);
    QVERIFY(dlg.selectRecord("Stat_A"));

    QTableView* table = dlg.findChild<QTableView*>();
    QVERIFY(table != nullptr);

    auto* proxyModel = qobject_cast<QSortFilterProxyModel*>(table->model());
    QVERIFY(proxyModel != nullptr);
    QCOMPARE(proxyModel->rowCount(), 2);

    // Sort ascending by Editor ID (column 0): Stat_A before Stat_Z
    table->sortByColumn(0, Qt::AscendingOrder);
    QCOMPARE(proxyModel->data(proxyModel->index(0, 0)).toString(), QStringLiteral("Stat_A"));
    QCOMPARE(proxyModel->data(proxyModel->index(1, 0)).toString(), QStringLiteral("Stat_Z"));

    // Sort descending by Editor ID (column 0): Stat_Z before Stat_A
    table->sortByColumn(0, Qt::DescendingOrder);
    QCOMPARE(proxyModel->data(proxyModel->index(0, 0)).toString(), QStringLiteral("Stat_Z"));
    QCOMPARE(proxyModel->data(proxyModel->index(1, 0)).toString(), QStringLiteral("Stat_A"));

    // Sort ascending by Form ID (column 1): 00010000 before 00020000
    table->sortByColumn(1, Qt::AscendingOrder);
    QCOMPARE(proxyModel->data(proxyModel->index(0, 0)).toString(), QStringLiteral("Stat_A"));
    QCOMPARE(proxyModel->data(proxyModel->index(1, 0)).toString(), QStringLiteral("Stat_Z"));

    // Sort descending by Form ID (column 1): 00020000 before 00010000
    table->sortByColumn(1, Qt::DescendingOrder);
    QCOMPARE(proxyModel->data(proxyModel->index(0, 0)).toString(), QStringLiteral("Stat_Z"));
    QCOMPARE(proxyModel->data(proxyModel->index(1, 0)).toString(), QStringLiteral("Stat_A"));
}

#include "test_objectwindow.moc"
QTEST_MAIN(TestObjectWindowModelPath)
