#ifndef RAWSUBRECORDWIDGET_HPP
#define RAWSUBRECORDWIDGET_HPP

#include <QWidget>

#include <QVector>

class QTableWidget;
struct RawSubRecord;

// Read-only inspector for a record's raw (unparsed) subrecords. Records
// whose specialized field parsing is still pending (EFSH, IMGS, etc.; see
// docs/REMAINING_WORK_PLAN.md Phase D/E) round-trip losslessly via
// rawSubRecords; this widget surfaces that data so users can see what's
// stored.
class RawSubrecordWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RawSubrecordWidget(QWidget* parent = nullptr);

    void setSubrecords(const QVector<RawSubRecord>& subrecords);
    int count() const;

private:
    QTableWidget* mTable;
};

#endif // RAWSUBRECORDWIDGET_HPP
