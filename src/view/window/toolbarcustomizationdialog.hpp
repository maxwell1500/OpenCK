#ifndef TOOLBARCUSTOMIZATIONDIALOG_HPP
#define TOOLBARCUSTOMIZATIONDIALOG_HPP

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QToolBar>
#include <QAction>

class ToolbarCustomizationDialog : public QDialog
{
    Q_OBJECT

public:
    struct ToolbarItem {
        QString actionName;
        QString displayText;
        bool visible;
    };

    explicit ToolbarCustomizationDialog(QToolBar* toolbar, QWidget* parent = nullptr);

    static void saveToolbarConfig(QToolBar* toolbar);
    static void restoreToolbarConfig(QToolBar* toolbar);

private slots:
    void moveUp();
    void moveDown();
    void toggleVisibility();
    void saveAndClose();

private:
    void populateList();
    QList<ToolbarItem> getAvailableActions() const;

    QToolBar* mToolbar;
    QListWidget* mListWidget;
    QPushButton* mUpBtn;
    QPushButton* mDownBtn;
    QPushButton* mToggleBtn;
    QPushButton* mSaveBtn;
};

#endif // TOOLBARCUSTOMIZATIONDIALOG_HPP
