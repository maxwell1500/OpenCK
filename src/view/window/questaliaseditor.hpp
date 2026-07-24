#ifndef QUESTALIASEDITOR_HPP
#define QUESTALIASEDITOR_HPP

#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QVector>
#include <QString>

struct QuestAlias
{
    QString name;
    int type = 0;
    quint32 refId = 0;
    bool optional = false;
};

class QuestRecord;

class QuestAliasEditor : public QDialog
{
    Q_OBJECT

public:
    QuestAliasEditor(QuestRecord* quest, QWidget* parent = nullptr);

private slots:
    void addAlias();
    void removeAlias();
    void saveChanges();

private:
    void setupUI();
    void loadFromQuest();
    void saveToQuest();

    QuestRecord* mQuest;
    QVector<QuestAlias> mAliases;

    QTableWidget* mAliasTable;
};

#endif // QUESTALIASEDITOR_HPP
