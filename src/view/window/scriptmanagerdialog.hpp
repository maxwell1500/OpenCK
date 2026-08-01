#ifndef SCRIPTMANAGERDIALOG_HPP
#define SCRIPTMANAGERDIALOG_HPP

#include <QDialog>
#include <QStringList>

class QListWidget;
class QLineEdit;
class QPushButton;

// Papyrus Script Manager: lists all .psc source files under the game's
// Scripts/Source directory (and its subfolders), with a filter box and
// actions to open / create a script.
class ScriptManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScriptManagerDialog(const QString& scriptsRoot, QWidget* parent = nullptr);

    // Returns the currently selected script path, or empty.
    QString selectedScript() const;

private slots:
    void onFilterChanged(const QString& text);
    void onOpen();
    void onNewScript();
    void onDoubleClick();

private:
    void populate();
    QString rootDir() const;

    QString m_scriptsRoot;
    QStringList m_allScripts;   // absolute paths
    QListWidget* m_list;
    QLineEdit* m_filterEdit;
    QPushButton* m_openBtn;
    QPushButton* m_newBtn;
};

#endif // SCRIPTMANAGERDIALOG_HPP
