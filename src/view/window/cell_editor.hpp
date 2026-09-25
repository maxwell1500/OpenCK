#ifndef CELL_EDITOR_HPP
#define CELL_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QVector>

#include "../../../libs/files/esm/cellreferencedata.hpp"

class Data;
class NifViewportWidget;
struct CellRecord;
struct FormPickerEntry;

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
    QVector<CellRefEntry> loadReferences() const;
    QVector<FormPickerEntry> loadFormEntries() const;
    bool applyReferenceChanges();
    NifViewportWidget* findViewport() const;

    Data* mData;
    CellRecord* mCell;

    QLineEdit* mEditorIdEdit;
    QLineEdit* mCellNameEdit;
    QSpinBox* mCellXSpin;
    QSpinBox* mCellYSpin;
    QSpinBox* mOwnerSpin;
    QSpinBox* mLockLevelSpin;
    QVector<CellRefEntry> mOriginalReferences;
    QVector<CellRefEntry> mEditedReferences;
    bool mReferencesEdited = false;
};

#endif // CELL_EDITOR_HPP
