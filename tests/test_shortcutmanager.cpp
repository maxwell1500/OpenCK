#include <QtTest>

#include "../../src/model/world/shortcutmanager.hpp"

class TestShortcutManager : public QObject
{
    Q_OBJECT

private slots:
    void testInit();
    void testGetSet();
    void testResetToDefaults();
    void testEntries();
    void testCategories();
};

void TestShortcutManager::testInit()
{
    ShortcutManager& mgr = ShortcutManager::instance();
    mgr.init();
    QVERIFY(!mgr.entries().isEmpty());
}

void TestShortcutManager::testGetSet()
{
    ShortcutManager& mgr = ShortcutManager::instance();
    mgr.init();

    QList<ShortcutManager::ShortcutEntry> allEntries = mgr.entries();
    if (allEntries.isEmpty()) return;

    QString firstName = allEntries.first().name;
    QKeySequence originalKey = mgr.get(firstName);

    QKeySequence newKey(Qt::CTRL | Qt::SHIFT | Qt::Key_Z);
    mgr.set(firstName, newKey);
    QCOMPARE(mgr.get(firstName), newKey);

    mgr.set(firstName, originalKey);
    QCOMPARE(mgr.get(firstName), originalKey);
}

void TestShortcutManager::testResetToDefaults()
{
    ShortcutManager& mgr = ShortcutManager::instance();
    mgr.init();

    QList<ShortcutManager::ShortcutEntry> allEntries = mgr.entries();
    if (allEntries.isEmpty()) return;

    QString firstName = allEntries.first().name;
    QKeySequence originalKey = mgr.get(firstName);

    QKeySequence newKey(Qt::CTRL | Qt::SHIFT | Qt::Key_Z);
    mgr.set(firstName, newKey);
    QCOMPARE(mgr.get(firstName), newKey);

    mgr.resetToDefaults();
    QCOMPARE(mgr.get(firstName), originalKey);
}

void TestShortcutManager::testEntries()
{
    ShortcutManager& mgr = ShortcutManager::instance();
    mgr.init();

    QList<ShortcutManager::ShortcutEntry> allEntries = mgr.entries();
    QVERIFY(!allEntries.isEmpty());

    for (const auto& entry : allEntries) {
        QVERIFY(!entry.name.isEmpty());
        QVERIFY(!entry.category.isEmpty());
    }
}

void TestShortcutManager::testCategories()
{
    ShortcutManager& mgr = ShortcutManager::instance();
    mgr.init();

    QStringList cats = mgr.categories();
    QVERIFY(!cats.isEmpty());

    for (const auto& cat : cats) {
        QList<ShortcutManager::ShortcutEntry> catEntries = mgr.entriesByCategory(cat);
        QVERIFY(!catEntries.isEmpty());
        for (const auto& entry : catEntries) {
            QCOMPARE(entry.category, cat);
        }
    }
}

#include "test_shortcutmanager.moc"
QTEST_MAIN(TestShortcutManager)
