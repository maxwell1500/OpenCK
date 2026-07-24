#ifndef CELLTRANSEDITION_HPP
#define CELLTRANSEDITION_HPP

#include <QDialog>
#include <QTreeWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>

class Data;
struct WorldspaceRecord;
struct CellRecord;

class CellTransitionsEditor : public QDialog
{
    Q_OBJECT

public:
    CellTransitionsEditor(Data* data, QWidget* parent = nullptr);
    ~CellTransitionsEditor();

private slots:
    void onNodeSelected(QTreeWidgetItem* item, int column);
    void onAddTransition();
    void onEditTransition();
    void onDeleteTransition();
    void onSave();

private:
    void setupUI();
    void loadWorldspaces();
    void refreshTree();
    void showWorldspaceDetails(const WorldspaceRecord* ws);
    void showCellDetails(const CellRecord* cell);
    void showTransitionDetails(int fromCell, int toCell);

    Data* mData;
    QTreeWidget* mTree;
    QTextEdit* mDetailEdit;
    QPushButton* mAddTransitionButton;
    QPushButton* mEditButton;
    QPushButton* mDeleteButton;
    QPushButton* mSaveButton;
    QLabel* mStatusLabel;

    WorldspaceRecord* mSelectedWorldspace;
    CellRecord* mSelectedCell;
    int mSelectedFromCell;
    int mSelectedToCell;
};

#endif // CELLTRANSEDITION_HPP
