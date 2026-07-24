#ifndef SHORTCUTMANAGER_HPP
#define SHORTCUTMANAGER_HPP

#include <QKeySequence>
#include <QMap>
#include <QString>

/// \brief Singleton manager for application keyboard shortcuts
/// 
/// ShortcutManager provides centralized management of keyboard shortcuts
/// throughout the application. It supports:
/// - Default shortcut definitions for all actions
/// - User-customizable shortcuts persisted to editor.ini
/// - Category-based organization (File, Edit, View, etc.)
/// - Reset to defaults functionality
/// 
/// Configuration is stored in editor.ini under [Shortcuts] section.
/// 
/// Usage:
/// \code
/// ShortcutManager& manager = ShortcutManager::instance();
/// manager.init();  // Call once at startup
/// QKeySequence key = manager.get("File.Open");
/// manager.set("File.Open", QKeySequence("Ctrl+O"));
/// \endcode
class ShortcutManager
{
public:
    /// \brief Structure representing a single shortcut definition
    struct ShortcutEntry {
        QString name;                ///< Action name (e.g., "File.Open")
        QString category;            ///< Category (e.g., "File", "Edit")
        QKeySequence defaultKey;     ///< Original default key binding
        QKeySequence currentKey;     ///< Current key binding (may be customized)
    };

    /// \brief Get singleton instance
    /// \return Reference to the global ShortcutManager instance
    static ShortcutManager& instance();

    /// \brief Initialize with all default shortcuts
    /// 
    /// Registers all built-in shortcuts with their default key bindings.
    /// Must be called before any get() or set() operations.
    void init();
    
    /// \brief Load shortcuts from editor.ini
    /// 
    /// Reads user-customized shortcuts from configuration file.
    /// Overwrites currentKey values with saved values.
    void loadFromIni();
    
    /// \brief Save current shortcuts to editor.ini
    /// 
    /// Writes all currentKey values to configuration file.
    /// Called automatically when user changes shortcuts in UI.
    void saveToIni();

    /// \brief Get shortcut key sequence by name
    /// \param name Action name (e.g., "File.Open", "Edit.Copy")
    /// \return QKeySequence for the action, or empty if not found
    QKeySequence get(const QString& name) const;
    
    /// \brief Set shortcut key sequence by name
    /// \param name Action name (e.g., "File.Open")
    /// \param key New key sequence to assign
    /// 
    /// Updates currentKey and marks as modified (not yet saved).
    /// Call saveToIni() to persist changes.
    void set(const QString& name, const QKeySequence& key);
    
    /// \brief Reset all shortcuts to defaults
    void resetToDefaults();
    
    /// \brief Reset shortcuts in a category to defaults
    /// \param category Category name (e.g., "File", "Edit")
    void resetCategory(const QString& category);

    /// \brief Get all shortcut entries
    QList<ShortcutEntry> entries() const;
    
    /// \brief Get shortcut entries filtered by category
    QList<ShortcutEntry> entriesByCategory(const QString& category) const;
    
    /// \brief Get list of all categories
    QStringList categories() const;

    /// \brief INI configuration section name
    static constexpr const char* iniGroup = "Shortcuts";

private:
    ShortcutManager() = default;
    
    /// \brief Register a shortcut with default key binding
    void registerShortcut(const QString& name, const QString& category, const QKeySequence& defaultKey);

    QMap<QString, ShortcutEntry> mShortcuts;
};

#endif // SHORTCUTMANAGER_HPP
