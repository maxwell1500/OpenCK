#ifndef CELLREFERENCEEDITOR_HPP
#define CELLREFERENCEEDITOR_HPP

#include <QDialog>

#include "../../../libs/files/esm/cellreferencedata.hpp"

#include <QVector>

class QTableWidget;
class QLineEdit;
class QDoubleSpinBox;
class QCheckBox;
class QPushButton;
struct CellRecord;

class CellReferenceEditor : public QDialog
{
    Q_OBJECT

public:
    CellReferenceEditor(CellRecord* cell, QWidget* parent = nullptr);

    QVector<CellRefEntry> getReferences() const { return mReferences; }

private slots:
    void addReference();
    void removeSelected();
    void saveReferences();

private:
    void setupUI();
    void loadFromCell(const CellRecord& cell);
    void populateTable();
    void setRowFromReference(int row, const CellRefEntry& ref);
    CellRefEntry getReferenceFromRow(int row) const;

    CellRecord* mCell;
    QVector<CellRefEntry> mReferences;

    QTableWidget* mTable;
    QPushButton* mAddBtn;
    QPushButton* mRemoveBtn;
    QPushButton* mSaveBtn;
    QPushButton* mCancelBtn;
};

#endif // CELLREFERENCEEDITOR_HPP
