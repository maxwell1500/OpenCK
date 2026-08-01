#ifndef OPALLIST_HPP
#define OPALLIST_HPP

#include <QString>
#include <QStringList>
#include <QVector>

// Parses OPAL procedural placement lists (.opl). The real format is
// comma-separated values with a header row; each data row describes one
// placement entry (form IDs, count, rules). Fields that are empty or that
// fail to parse are preserved as strings so callers can interpret them.
struct OpalList
{
    QStringList headers;
    QVector<QVector<QString>> rows;   // parallel to headers

    // Parses .opl content (header line + data rows). Returns the parsed
    // list; rows shorter than the header are padded with empty strings.
    static OpalList parse(const QString& content);

    // Loads and parses the given .opl file. Returns false if the file
    // cannot be read.
    static bool loadFile(const QString& path, OpalList& out);

    // Column value lookup helper: returns the value of the named column in
    // row i, or an empty string if absent.
    QString value(int row, const QString& columnName) const;

    int rowCount() const { return rows.size(); }
};

#endif // OPALLIST_HPP
