#include "rawsubrecordwidget.hpp"

#include "../../../libs/files/esm/records.hpp"

#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

QString formatName(NAME name)
{
    char buf[5] = { 0, 0, 0, 0, 0 };
    buf[0] = static_cast<char>((name >> 24) & 0xFF);
    buf[1] = static_cast<char>((name >> 16) & 0xFF);
    buf[2] = static_cast<char>((name >> 8) & 0xFF);
    buf[3] = static_cast<char>(name & 0xFF);
    return QString::fromLatin1(buf);
}

QString formatHex(const QByteArray& data)
{
    QString hex = QString::fromLatin1(data.toHex(' ').toUpper());
    if (hex.size() > 120)
        hex = hex.left(117) + "...";
    return hex;
}

}

RawSubrecordWidget::RawSubrecordWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    mTable = new QTableWidget(this);
    mTable->setColumnCount(3);
    mTable->setHorizontalHeaderLabels({"Subrecord", "Size", "Data (hex)"});
    mTable->horizontalHeader()->setStretchLastSection(true);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mTable->verticalHeader()->setVisible(false);
    layout->addWidget(mTable);
}

void RawSubrecordWidget::setSubrecords(const QVector<RawSubRecord>& subrecords)
{
    mTable->setRowCount(0);
    for (const auto& sub : subrecords)
    {
        const int row = mTable->rowCount();
        mTable->insertRow(row);
        mTable->setItem(row, 0, new QTableWidgetItem(formatName(sub.name)));
        mTable->setItem(row, 1, new QTableWidgetItem(QString::number(sub.data.size())));
        mTable->setItem(row, 2, new QTableWidgetItem(formatHex(sub.data)));
    }
}

int RawSubrecordWidget::count() const
{
    return mTable->rowCount();
}
