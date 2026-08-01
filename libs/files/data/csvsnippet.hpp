#ifndef CSVS_NIPPET_HPP
#define CSVS_NIPPET_HPP

#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonObject>
#include <QVariantMap>

// CSV Snippets: a column-based import template system matching the real
// Creation Kit's Snippets\*.txt workflow. A snippet file is one CSV row per
// output line. Rows may reference:
//   - `.Import=path`  : a nested template file whose lines are inlined here
//   - `<Type:Field>`  : a field accessor resolved from the record's JSON view
//                       (e.g. <NPC_:Level>), with optional nested sub-field
//                       path and a `.Count` suffix for array lengths.
// Fields that don't resolve render empty. Lines starting with `#` are
// comments and are skipped.
struct CsvSnippet
{
    // One parsed output row: the text with all accessors substituted, plus
    // the list of values substituted (for diagnostics).
    struct Row
    {
        QString text;
        QStringList substitutedValues;
    };

    // Resolves a record's fields into a flat map (formId, editorId, plus
    // every JSON field of the record). Used by render().
    static QVariantMap flattenRecord(const QJsonObject& record);

    // Renders a snippet template file, inlining .Import= directives
    // (maxDepth guards against recursive imports) and substituting
    // <Type:Field...> accessors against the given record JSON.
    static QVector<Row> render(const QString& snippetPath,
                               const QJsonObject& record,
                               int maxDepth = 8);

    // Inlines a .Import= directive by reading the referenced file. Returns
    // false if the file cannot be read.
    static bool inlineImport(const QString& snippetDir,
                             const QString& importPath,
                             const QJsonObject& record,
                             QVector<Row>& out,
                             int depth);

    // Substitutes every <...> accessor in a single line.
    static QString substituteLine(const QString& line, const QVariantMap& fields);

private:
    static QVariant resolveAccessor(const QString& accessor, const QVariantMap& fields);
};

#endif // CSVS_NIPPET_HPP
