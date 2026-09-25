#ifndef CELLREFERENCEEDITOR_HPP
#define CELLREFERENCEEDITOR_HPP

#include <QDialog>

#include "../../../libs/files/esm/cellreferencedata.hpp"
#include "formideditorwidget.hpp"

#include <QVector>

class QTableWidget;
class QLineEdit;
class QDoubleSpinBox;
class QCheckBox;
class QPushButton;

class CellReferenceEditor : public QDialog
{
    Q_OBJECT

public:
    CellReferenceEditor(const QVector<CellRefEntry>& references,
                        const QVector<FormPickerEntry>& formEntries,
                        QWidget* parent = nullptr);

    QVector<CellRefEntry> getReferences() const { return mReferences; }

private slots:
    void addReference();
    void removeSelected();
    void saveReferences();

private:
    void setupUI();
    void populateTable();
    void setRowFromReference(int row, const CellRefEntry& ref);
    CellRefEntry getReferenceFromRow(int row) const;

    QVector<CellRefEntry> mReferences;
    QVector<FormPickerEntry> mFormEntries;

    QTableWidget* mTable;
    QPushButton* mAddBtn;
    QPushButton* mRemoveBtn;
    QPushButton* mSaveBtn;
    QPushButton* mCancelBtn;
};

#endif // CELLREFERENCEEDITOR_HPP
