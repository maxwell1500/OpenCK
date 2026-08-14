#ifndef WEATHERLIGHTEDITOR_HPP
#define WEATHERLIGHTEDITOR_HPP

#include <QDialog>
#include <QTreeWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>

class Data;
struct GameSetting;
struct GlobalVariable;

class WeatherLightEditor : public QDialog
{
    Q_OBJECT

public:
    WeatherLightEditor(Data* data, QWidget* parent = nullptr);
    ~WeatherLightEditor();

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
};

#endif // WEATHERLIGHTEDITOR_HPP
