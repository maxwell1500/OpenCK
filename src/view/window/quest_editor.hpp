#ifndef QUEST_EDITOR_HPP
#define QUEST_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QPushButton>

class Data;
struct QuestRecord;

class QuestEditor : public QDialog
{
    Q_OBJECT

public:
    QuestEditor(Data* data, QuestRecord* quest, QWidget* parent = nullptr);

private slots:
    void saveRecord();
    void openStageEditor();
    void openAliasEditor();

private:
    void setupUI();
    void loadFromQuest();
    bool validateQuest();

    Data* mData;
    QuestRecord* mQuest;

    QLineEdit* mEditorIdEdit;
    QLineEdit* mQuestNameEdit;
    QLineEdit* mQuestTypeEdit;
    QTextEdit* mQuestDescEdit;
    QLineEdit* mDialogueViewEdit;
    QSpinBox* mStartRankSpin;
    QPushButton* mStagesBtn;
    QPushButton* mAliasesBtn;
};

#endif // QUEST_EDITOR_HPP
