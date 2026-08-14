#ifndef WATEREDITOR_HPP
#define WATEREDITOR_HPP

#include <QDialog>
#include <QTreeWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>

class Data;
struct GlobalVariable;
struct GameSetting;

class WaterEditor : public QDialog
{
    Q_OBJECT

public:
    WaterEditor(Data* data, QWidget* parent = nullptr);
    ~WaterEditor();

private slots:
    void onNodeSelected(QTreeWidgetItem* item, int column);
    void onAddSetting();
    void onEditSetting();
    void onDeleteSetting();
    void onSave();

private:
    void setupUI();
    void loadSettings();
    void refreshTree();
    void showSettingDetails(const QString& name, const QString& value);

    Data* mData;
    QTreeWidget* mTree;
    QTextEdit* mDetailEdit;
    QPushButton* mAddSettingButton;
    QPushButton* mEditButton;
    QPushButton* mDeleteButton;
    QPushButton* mSaveButton;
    QLabel* mStatusLabel;

    QString mSelectedName;
    QString mSelectedValue;
    QString mSelectedType;
};

#endif // WATEREDITOR_HPP
