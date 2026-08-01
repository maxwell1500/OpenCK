#ifndef REPORTS_H
#define REPORTS_H

#include "../doc/messages.hpp"
#include "../world/ckid.hpp"

#include <QAbstractTableModel>
#include <QString>
#include <QVector>

class ReportModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ReportModel(QObject* parent = nullptr, bool fieldColumn = false, bool levelColumn = true);
    
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    
    virtual bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());
    void add(const Message& message);
    QString getHint(int row) const;
    const CkId getCkId(int row) const;

    int countErrors() const;
    void clear();

private:
    QVector<Message> rows;

    enum Columns
    {
        Column_Type = 0, 
        Column_Id = 1, 
        Column_Hint = 2,
        Columns = 3,
    };

    int columnDescription;
    int columnField;
    int columnLevel;
};

// xEdit-style analysis report export. Writes the message list as a
// tab-separated report (Level / Type / ID / Message / Hint) so users can
// open it in a spreadsheet or diff it across plugin versions.
namespace ReportExport
{
    // Serializes the given messages to a tab-separated text block
    // (one record per line, no trailing newline).
    QString messagesToText(const QVector<Message>& messages);

    // Writes messages to the given file path. Returns true on success.
    bool exportMessages(const QString& filePath, const QVector<Message>& messages);
}

#endif // REPORTS_H