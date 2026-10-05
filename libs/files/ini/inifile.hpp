#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

// A tolerant reader for the plain-text INI files game tools ship alongside their
// executables. It exists because there was no reusable parser in the tree: the one
// hand-rolled scanner (MasterManagement::loadIni) is scoped to a single section,
// has no production caller, and cannot represent a key that appears more than
// once -- which the archive configuration below relies on.
//
// Deliberately not QSettings. QSettings is fine for a file we own, but it
// normalises and reinterprets keys and arrays on the way out, and these files are
// read for interoperability: what is on disk is what has to be understood.
class IniFile
{
public:
    // Returns false only when the file could not be opened or read. A file that
    // opens but contains nothing we recognise is a successful, empty load -- an
    // absent section must not read as a broken file.
    bool load(const QString& path);

    bool isEmpty() const { return m_sections.isEmpty(); }
    QStringList sections() const;

    // Section and key lookups are case-insensitive because these files are not
    // consistent about it: the same [Archive] section carries both
    // "sResourceIndexFileList" and "SResourceArchiveList".
    bool hasSection(const QString& section) const;
    bool hasKey(const QString& section, const QString& key) const;

    // First value for the key, or a default. An empty string is returned as-is
    // when it is genuinely empty, so callers cannot tell "absent" from "blank"
    // through this overload -- use hasKey() when that matters.
    QString value(const QString& section, const QString& key,
                  const QString& defaultValue = QString()) const;

    // Every value written for the key, in file order. An INI may legitimately
    // repeat a key; this is the only accessor that does not lose those.
    QStringList values(const QString& section, const QString& key) const;

    // A comma-separated value split into trimmed, non-empty parts. These lists are
    // written "A.ba2, B.ba2, C.ba2", with spaces after the commas, and a trailing
    // comma is not unheard of.
    QStringList valueList(const QString& section, const QString& key) const;

    // Numeric value of the first entry, or defaultValue when absent or unparsable.
    // Kept separate from value() so a malformed setting cannot silently become 0.
    int intValue(const QString& section, const QString& key, int defaultValue = 0) const;
    bool boolValue(const QString& section, const QString& key, bool defaultValue = false) const;

private:
    // section (lower) -> key (lower) -> values in file order
    QMap<QString, QMap<QString, QStringList>> m_sections;
};