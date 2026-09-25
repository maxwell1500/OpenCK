#ifndef AIPACKAGEEDITOR_HPP
#define AIPACKAGEEDITOR_HPP

#include <QDialog>
#include <QTreeWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <functional>

class Data;
struct PackageRecord;

class AIPackageEditor : public QDialog
{
    Q_OBJECT

public:
    AIPackageEditor(Data* data, std::function<bool()> saveCallback = {},
                    QWidget* parent = nullptr);
    ~AIPackageEditor();

private slots:
    void onNodeSelected(QTreeWidgetItem* item, int column);
    void onAddPackage();
    void onEditPackage();
    void onDeletePackage();
    void onSave();

private:
    void setupUI();
    void loadPackages();
    void refreshTree();
    void showPackageDetails(const PackageRecord* pack);

    Data* mData;
    std::function<bool()> mSaveCallback;
    QTreeWidget* mTree;
    QTextEdit* mDetailEdit;
    QPushButton* mAddPackageButton;
    QPushButton* mEditButton;
    QPushButton* mDeleteButton;
    QPushButton* mSaveButton;
    QLabel* mStatusLabel;

    PackageRecord* mSelectedPack;
};

#endif // AIPACKAGEEDITOR_HPP
