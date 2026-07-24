#ifndef DIALOGUETREEEDITOR_HPP
#define DIALOGUETREEEDITOR_HPP

#include "../../model/world/data.hpp"

#include <QDialog>
#include <QTreeWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>

class Data;

class DialogueTreeEditor : public QDialog
{
    Q_OBJECT

public:
    explicit DialogueTreeEditor(Data* data, QWidget* parent = nullptr);
    ~DialogueTreeEditor();

private slots:
    void onNodeSelected(QTreeWidgetItem* item, int column);
    void onAddDial();
    void onAddInfo();
    void onEditNode();
    void onDeleteNode();
    void onSave();

private:
    void setupUI();
    void loadDialogueTree();
    void refreshTree();
    void showDialDetails(const DialRecord* dial);
    void showInfoDetails(const InfoRecord* info);
    QString getTreeWidgetItemText(QTreeWidgetItem* item) const;
    int getTreeWidgetItemType(QTreeWidgetItem* item) const;

    Data* mData;
    QTreeWidget* mTree;
    QTextEdit* mDetailEdit;
    QLineEdit* mSearchEdit;
    QPushButton* mAddDialButton;
    QPushButton* mAddInfoButton;
    QPushButton* mEditButton;
    QPushButton* mDeleteButton;
    QPushButton* mSaveButton;
    QLabel* mStatusLabel;

    QList<DialRecord*> mSelectedDials;
    QList<InfoRecord*> mSelectedInfos;
};

#endif // DIALOGUETREEEDITOR_HPP
