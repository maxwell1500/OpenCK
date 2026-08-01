#ifndef SCRIPTFLAGFILE_HPP
#define SCRIPTFLAGFILE_HPP

#include <QString>
#include <QStringList>
#include <QVector>

// Parser for Papyrus script flag files (.flg). The real format is one
// mapping per line: "<scriptName> = <Flag1>|<Flag2>|..." with ';' comment
// lines. The values are the documented Creation Kit flags: Hidden,
// Conditional, Default, CollapsedOnRef, CollapsedOnBase, Mandatory.
struct ScriptFlagFile
{
    struct Entry
    {
        QString scriptName;
        QStringList flags;
    };

    // Parses .flg content into entries. Blank lines and lines starting
    // with ';' or '#' are ignored. Returns the parsed entries (empty if
    // nothing could be parsed).
    static QVector<Entry> parse(const QString& content);

    // Loads and parses the given file. Returns false if the file cannot
    // be read.
    static bool loadFile(const QString& path, QVector<Entry>& out);

    // The canonical set of known script property flags.
    static QStringList knownFlags();

    // Validates every flag in every entry against knownFlags(); returns a
    // list of "<script>: <flag>" for unknown flags found.
    static QStringList unknownFlags(const QVector<Entry>& entries);
};

#endif // SCRIPTFLAGFILE_HPP
