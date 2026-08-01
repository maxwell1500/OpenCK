#ifndef REFERENCEBATCHDIALOG_HPP
#define REFERENCEBATCHDIALOG_HPP

#include <QDialog>

#include <QVector>

struct CellRefEntry;

class QDoubleSpinBox;
class QCheckBox;
class QComboBox;
class QPushButton;

// "Reference batch action" window: applies a chosen operation to every
// selected cell reference at once. Returns the modified list via
// getReferences().
class ReferenceBatchDialog : public QDialog
{
    Q_OBJECT

public:
    ReferenceBatchDialog(const QVector<CellRefEntry>& refs, QWidget* parent = nullptr);

    QVector<CellRefEntry> getReferences() const { return mRefs; }

private slots:
    void onApply();
    void onReset();

private:
    void setupUI();
    void applyOperation();

    QVector<CellRefEntry> mRefs;
    QVector<CellRefEntry> mOriginal;

    QComboBox* mOperationCombo;
    QDoubleSpinBox* mValueSpin;
    QCheckBox* mFlagOnCheck;
};

#endif // REFERENCEBATCHDIALOG_HPP
