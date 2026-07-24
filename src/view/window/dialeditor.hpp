#ifndef DIALEDITOR_HPP
#define DIALEDITOR_HPP

#include <QDialog>

class Data;
class QDialogButtonBox;
class QGroupBox;
class QTableView;
class QTableWidget;
class QLineEdit;
class QPushButton;
class QTextEdit;
class QLabel;
class QComboBox;
class DialRecord;

class DialEditor : public QDialog
{
    Q_OBJECT

public:
    explicit DialEditor(Data* data, QWidget* parent = nullptr);
    ~DialEditor();

    void loadRecord(DialRecord* record);
    bool saveRecord();

    bool isNewRecord() const { return isNew; }

private:
    bool validate();
    void updateResponseDetails();

private slots:
    void onAddTopic();
    void onRemoveTopic();
    void onTopicSelected(QModelIndex current);
    void onSave();
    void onBrowseVoiceFile();
    void onAddCondition();
    void onRemoveCondition();

private:
    void setupUI();
    void loadTopics();
    void saveTopics();
    void loadConditions();
    void saveConditions();

    Data* mData;
    DialRecord* record;
    bool isNew;

    QTableView* topicListView;
    QLineEdit* editorIdEdit;
    QLineEdit* nameEdit;
    QPushButton* addTopicButton;
    QPushButton* removeTopicButton;
    QTextEdit* descriptionEdit;
    QTextEdit* responseTextEdit;
    QComboBox* emotionTypeCombo;
    QLineEdit* targetIdEdit;
    QLineEdit* voiceFileEdit;
    QPushButton* browseVoiceFileBtn;
    QTableWidget* conditionTable;
    QPushButton* addConditionBtn;
    QPushButton* removeConditionBtn;
    QDialogButtonBox* buttonBox;
};

#endif // DIALEDITOR_HPP
