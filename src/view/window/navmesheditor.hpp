#ifndef NAVMESHEDITOR_HPP
#define NAVMESHEDITOR_HPP

#include <QDialog>
#include <QTreeWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>

class Data;
struct CellRecord;
class NavmeshEditorDialog;
class NifViewportWidget;

class NavmeshEditor : public QDialog
{
    Q_OBJECT

public:
    NavmeshEditor(Data* data, QWidget* parent = nullptr, NifViewportWidget* viewport = nullptr);
    ~NavmeshEditor();

private slots:
    void onNodeSelected(QTreeWidgetItem* item, int column);
    void onAddCell();
    void onEditCell();
    void onDeleteCell();
    void onSave();
    void onEditDetails();
    void onGenerateNavmesh();

private:
    void setupUI();
    void loadCells();
    void refreshTree();
    void showCellDetails(const CellRecord* cell);

    Data* mData;
    QTreeWidget* mTree;
    QTextEdit* mDetailEdit;
    QPushButton* mAddCellButton;
    QPushButton* mEditButton;
    QPushButton* mDeleteButton;
    QPushButton* mSaveButton;
    QPushButton* mEditDetailsButton;
    QPushButton* mGenerateButton;
    QLabel* mStatusLabel;

    CellRecord* mSelectedCell;
    NavmeshEditorDialog* mDetailDialog;
    NifViewportWidget* mViewport;
};

#endif // NAVMESHEDITOR_HPP
