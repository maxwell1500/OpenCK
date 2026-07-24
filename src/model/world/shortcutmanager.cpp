#include "shortcutmanager.hpp"
#include "shortcuts.hpp"
#include "logger.hpp"
#include "filepaths.hpp"

#include <QSettings>
#include <QCoreApplication>

ShortcutManager& ShortcutManager::instance()
{
    static ShortcutManager s;
    return s;
}

void ShortcutManager::init()
{
    // File Menu
    registerShortcut("NewPlugin",        "File", CKShortcuts::NewPlugin);
    registerShortcut("OpenPlugin",       "File", CKShortcuts::OpenPlugin);
    registerShortcut("SavePlugin",       "File", CKShortcuts::SavePlugin);
    registerShortcut("SaveAsPlugin",     "File", CKShortcuts::SaveAsPlugin);
    registerShortcut("ClosePlugin",      "File", CKShortcuts::ClosePlugin);
    registerShortcut("Exit",             "File", CKShortcuts::Exit);

    // Edit Menu
    registerShortcut("Undo",             "Edit", CKShortcuts::Undo);
    registerShortcut("Redo",             "Edit", CKShortcuts::Redo);
    registerShortcut("Cut",              "Edit", CKShortcuts::Cut);
    registerShortcut("Copy",             "Edit", CKShortcuts::Copy);
    registerShortcut("Paste",            "Edit", CKShortcuts::Paste);
    registerShortcut("Delete",           "Edit", CKShortcuts::Delete);
    registerShortcut("SelectAll",        "Edit", CKShortcuts::SelectAll);
    registerShortcut("Find",             "Edit", CKShortcuts::Find);
    registerShortcut("FindNext",         "Edit", CKShortcuts::FindNext);
    registerShortcut("FindPrevious",     "Edit", CKShortcuts::FindPrevious);
    registerShortcut("Duplicate",        "Edit", CKShortcuts::Duplicate);
    registerShortcut("SearchAndReplace", "Edit", CKShortcuts::SearchAndReplace);

    // View Menu
    registerShortcut("ObjectWindow",     "View", CKShortcuts::ObjectWindow);
    registerShortcut("NifViewport",      "View", CKShortcuts::NifViewport);
    registerShortcut("ScriptEditor",     "View", CKShortcuts::ScriptEditor);
    registerShortcut("DialogueEditor",   "View", CKShortcuts::DialogueEditor);
    registerShortcut("DialogueTree",     "View", CKShortcuts::DialogueTree);
    registerShortcut("QuestGraph",       "View", CKShortcuts::QuestGraph);
    registerShortcut("AIPackages",       "View", CKShortcuts::AIPackages);
    registerShortcut("WeatherLight",     "View", CKShortcuts::WeatherLight);
    registerShortcut("Navmesh",          "View", CKShortcuts::Navmesh);
    registerShortcut("WaterEditor",      "View", CKShortcuts::WaterEditor);
    registerShortcut("CellTransitions",  "View", CKShortcuts::CellTransitions);
    registerShortcut("MaterialEditor",   "View", CKShortcuts::MaterialEditor);
    registerShortcut("PapyrusDebugger",  "View", CKShortcuts::PapyrusDebugger);
    registerShortcut("FormIdEditor",     "View", CKShortcuts::FormIdEditor);
    registerShortcut("Refresh",          "View", CKShortcuts::Refresh);

    // World Menu
    registerShortcut("Worldspaces",      "World", CKShortcuts::Worldspaces);
    registerShortcut("Cells",            "World", CKShortcuts::Cells);
    registerShortcut("LandscapeEditing", "World", CKShortcuts::LandscapeEditing);
    registerShortcut("ObjectPalette",    "World", CKShortcuts::ObjectPalette);

    // Plugins Menu
    registerShortcut("LoadOrder",          "Plugins", CKShortcuts::LoadOrder);
    registerShortcut("MasterFiles",        "Plugins", CKShortcuts::MasterFiles);
    registerShortcut("ConflictDetection",  "Plugins", CKShortcuts::ConflictDetection);
    registerShortcut("ConflictResolution", "Plugins", CKShortcuts::ConflictResolution);
    registerShortcut("PluginMerge",        "Plugins", CKShortcuts::PluginMerge);
    registerShortcut("LoadOrderOptimizer", "Plugins", CKShortcuts::LoadOrderOptimizer);
    registerShortcut("BashedPatch",        "Plugins", CKShortcuts::BashedPatch);
    registerShortcut("ExternalTools",      "Plugins", CKShortcuts::ExternalTools);

    // Utility
    registerShortcut("Preferences",     "Utility", CKShortcuts::Preferences);
    registerShortcut("Validate",         "Utility", CKShortcuts::Validate);
    registerShortcut("ToggleGrid",       "Utility", CKShortcuts::ToggleGrid);
    registerShortcut("ToggleBoundingBoxes", "Utility", CKShortcuts::ToggleBoundingBoxes);

    // Navigation
    registerShortcut("NextRecord",       "Navigation", CKShortcuts::NextRecord);
    registerShortcut("PreviousRecord",   "Navigation", CKShortcuts::PreviousRecord);
    registerShortcut("FirstRecord",      "Navigation", CKShortcuts::FirstRecord);
    registerShortcut("LastRecord",       "Navigation", CKShortcuts::LastRecord);
    registerShortcut("ExpandAll",        "Navigation", CKShortcuts::ExpandAll);
    registerShortcut("CollapseAll",      "Navigation", CKShortcuts::CollapseAll);

    // Help
    registerShortcut("About",            "Help", CKShortcuts::About);

    loadFromIni();

    LOG_INFO(QString("ShortcutManager initialized with %1 shortcuts").arg(mShortcuts.size()));
}

void ShortcutManager::registerShortcut(const QString& name, const QString& category, const QKeySequence& defaultKey)
{
    ShortcutEntry entry;
    entry.name = name;
    entry.category = category;
    entry.defaultKey = defaultKey;
    entry.currentKey = defaultKey;
    mShortcuts[name] = entry;
}

void ShortcutManager::loadFromIni()
{
    QString configPath = FilePaths::configFilePath();
    QSettings conf(configPath, QSettings::IniFormat);
    conf.beginGroup(iniGroup);

    for (auto it = mShortcuts.begin(); it != mShortcuts.end(); ++it)
    {
        QString saved = conf.value(it.key(), QString()).toString();
        if (!saved.isEmpty())
        {
            it.value().currentKey = QKeySequence(saved);
        }
    }

    conf.endGroup();
}

void ShortcutManager::saveToIni()
{
    QString configPath = FilePaths::configFilePath();
    QSettings conf(configPath, QSettings::IniFormat);
    conf.beginGroup(iniGroup);
    conf.remove("");

    for (auto it = mShortcuts.begin(); it != mShortcuts.end(); ++it)
    {
        if (it.value().currentKey != it.value().defaultKey)
        {
            conf.setValue(it.key(), it.value().currentKey.toString());
        }
    }

    conf.endGroup();
    conf.sync();

    LOG_INFO("Shortcuts saved to INI");
}

QKeySequence ShortcutManager::get(const QString& name) const
{
    if (mShortcuts.contains(name))
        return mShortcuts[name].currentKey;
    return QKeySequence();
}

void ShortcutManager::set(const QString& name, const QKeySequence& key)
{
    if (mShortcuts.contains(name))
    {
        mShortcuts[name].currentKey = key;
    }
}

void ShortcutManager::resetToDefaults()
{
    for (auto it = mShortcuts.begin(); it != mShortcuts.end(); ++it)
    {
        it.value().currentKey = it.value().defaultKey;
    }
}

void ShortcutManager::resetCategory(const QString& category)
{
    for (auto it = mShortcuts.begin(); it != mShortcuts.end(); ++it)
    {
        if (it.value().category == category)
        {
            it.value().currentKey = it.value().defaultKey;
        }
    }
}

QList<ShortcutManager::ShortcutEntry> ShortcutManager::entries() const
{
    return mShortcuts.values();
}

QList<ShortcutManager::ShortcutEntry> ShortcutManager::entriesByCategory(const QString& category) const
{
    QList<ShortcutEntry> result;
    for (const auto& entry : mShortcuts)
    {
        if (entry.category == category)
            result.append(entry);
    }
    return result;
}

QStringList ShortcutManager::categories() const
{
    QSet<QString> cats;
    for (const auto& entry : mShortcuts)
        cats.insert(entry.category);
    return cats.values();
}
