#ifndef INFOEDITOR_HPP
#define INFOEDITOR_HPP

#include <QDialog>

class Data;
class QDialogButtonBox;
class QTableView;
class QLineEdit;
class QPushButton;
class QTextEdit;
class QLabel;
class InfoRecord;

class InfoEditor : public QDialog
{
    Q_OBJECT

public:
    explicit InfoEditor(Data* data, QWidget* parent = nullptr);
    ~InfoEditor();

    void loadRecord(InfoRecord* record);
    bool saveRecord();

    bool isNewRecord() const { return isNew; }

private:
    bool validate();

private slots:
    void onAddResponse();
    void onRemoveResponse();
    void onResponseSelected(QModelIndex current);
    void onSave();

private:
    void setupUI();
    void loadResponses();
    void saveResponses();

    Data* mData;
    InfoRecord* record;
    bool isNew;

    QTableView* responseListView;
    QLineEdit* editorIdEdit;
    QPushButton* addResponseButton;
    QPushButton* removeResponseButton;
    QTextEdit* responseTextEdit;
    QTextEdit* scriptEdit;
    QDialogButtonBox* buttonBox;
};

#endif // INFOEDITOR_HPP
