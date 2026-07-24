#ifndef CELL_EDITOR_HPP
#define CELL_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>

class Data;
class NifViewportWidget;
struct CellRecord;

class CellEditor : public QDialog
{
    Q_OBJECT

public:
    CellEditor(Data* data, CellRecord* cell, QWidget* parent = nullptr);

private slots:
    void saveRecord();
    void openReferences();

private:
    bool validate();
    void setupUI();
    void loadFromCell();
    NifViewportWidget* findViewport() const;

    Data* mData;
    CellRecord* mCell;

    QLineEdit* mEditorIdEdit;
    QLineEdit* mCellNameEdit;
    QSpinBox* mCellXSpin;
    QSpinBox* mCellYSpin;
    QSpinBox* mOwnerSpin;
    QSpinBox* mLockLevelSpin;
};

#endif // CELL_EDITOR_HPP
