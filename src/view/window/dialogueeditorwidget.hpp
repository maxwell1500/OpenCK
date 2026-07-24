#ifndef DIALOGUEEDITORWIDGET_HPP
#define DIALOGUEEDITORWIDGET_HPP

#include <QWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class Data;
class DialRecord;
class InfoRecord;

class DialogueEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DialogueEditorWidget(Data* data, QWidget* parent = nullptr);
    ~DialogueEditorWidget();

    void loadDialogue(const QString& dialId);
    void saveDialogue();

private slots:
    void onTreeSelectionChanged();
    void onEditCondition();
    void onAddInfo();
    void onRemoveInfo();

private:
    void setupUI();
    void populateTree();
    void updateEditor();

    Data* mData;
    QTreeWidget* treeWidget;
    QTextEdit* conditionEditor;
    QLineEdit* topicEdit;
    QPushButton* editConditionButton;
    QPushButton* addInfoButton;
    QPushButton* removeInfoButton;
    
    QString currentDialId;
};

#endif // DIALOGUEEDITORWIDGET_HPP
