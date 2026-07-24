#ifndef QUESTGRAPHEDITOR_HPP
#define QUESTGRAPHEDITOR_HPP

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

class QuestGraphEditor : public QDialog
{
    Q_OBJECT

public:
    explicit QuestGraphEditor(Data* data, QWidget* parent = nullptr);
    ~QuestGraphEditor();

private slots:
    void onNodeSelected(QTreeWidgetItem* item, int column);
    void onAddStage();
    void onAddObjective();
    void onEditNode();
    void onDeleteNode();
    void onSave();

private:
    void setupUI();
    void loadQuestGraph();
    void refreshTree();
    void showQuestDetails(const QuestRecord* quest);
    void showStageDetails(int stageIndex);
    void showObjectiveDetails(int objectiveIndex);
    void showAliasDetails(int aliasIndex);

    Data* mData;
    QTreeWidget* mTree;
    QTextEdit* mDetailEdit;
    QLineEdit* mSearchEdit;
    QPushButton* mAddStageButton;
    QPushButton* mAddObjectiveButton;
    QPushButton* mEditButton;
    QPushButton* mDeleteButton;
    QPushButton* mSaveButton;
    QLabel* mStatusLabel;

    QuestRecord* mSelectedQuest;
    int mSelectedStageIndex;
    int mSelectedObjectiveIndex;
    int mSelectedAliasIndex;
};

#endif // QUESTGRAPHEDITOR_HPP
