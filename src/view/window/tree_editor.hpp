#ifndef TREE_EDITOR_HPP
#define TREE_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QGroupBox>

class Data;
struct TreeRecord;

class TreeEditor : public QDialog
{
    Q_OBJECT

public:
    TreeEditor(Data* data, TreeRecord* tree, QWidget* parent = nullptr);

private slots:
    void saveRecord();

private:
    bool validate();
    void setupUI();
    void loadFromTree();
    void saveToTree();

    Data* mData;
    TreeRecord* mTree;
    QLineEdit* mEditorIdEdit;
    QLineEdit* mModelEdit;
};

#endif // TREE_EDITOR_HPP
