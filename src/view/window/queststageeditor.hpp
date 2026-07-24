#ifndef QUESTSTAGEEDITOR_HPP
#define QUESTSTAGEEDITOR_HPP

#include <QDialog>
#include <QTableWidget>
#include <QListWidget>
#include <QSpinBox>
#include <QLineEdit>
#include <QVector>

struct QuestRecord;

class QuestStageEditor : public QDialog
{
    Q_OBJECT

public:
    QuestStageEditor(QuestRecord* quest, QWidget* parent = nullptr);

private slots:
    void addStage();
    void removeStage();
    void addItemReward();
    void removeItemReward();
    void addSpellReward();
    void removeSpellReward();
    void saveChanges();

private:
    void setupUI();
    void loadFromQuest();
    void saveToQuest();

    QuestRecord* mQuest;

    QVector<quint32> mStageIndices;
    QVector<QString> mStageTexts;
    QVector<QString> mObjectiveTexts;
    QVector<quint32> mStageFlags;
    quint32 mXpReward;
    quint32 mGoldReward;
    QVector<quint32> mItemRewards;
    QVector<quint32> mSpellRewards;

    QTableWidget* mStagesTable;
    QSpinBox* mXpSpin;
    QSpinBox* mGoldSpin;
    QListWidget* mItemList;
    QListWidget* mSpellList;
    QLineEdit* mItemFormIdEdit;
    QLineEdit* mSpellFormIdEdit;
};

#endif // QUESTSTAGEEDITOR_HPP
