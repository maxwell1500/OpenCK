#pragma once

#include <QSet>
#include <QString>
#include <QVector>

#include "obscriptparser.hpp"

// Semantic pass over a parsed OBScript program: builds a symbol table of
// functions / parameters / locals / globals, reports duplicate declarations,
// and collects identifiers that do not resolve to a declared symbol. Unknown
// identifiers are returned in `referencedExternals` (they are typically the
// game's native functions/properties, whose catalog is not part of the script);
// callers may pass a `knownExternals` set to suppress ones they know.
namespace ObScript
{

enum class BindSymbolKind
{
    Function,
    Parameter,
    Local,
    Global
};

struct BindSymbol
{
    QString name;
    BindSymbolKind kind = BindSymbolKind::Local;
    int line = 1;
    int paramCount = 0;
};

enum class BindSeverity
{
    Warning,
    Error
};

struct BindDiagnostic
{
    BindSeverity severity = BindSeverity::Error;
    QString message;
    int line = 1;
};

struct BindResult
{
    bool ok = true;
    QVector<BindSymbol> symbols;
    QVector<BindDiagnostic> diagnostics;
    QSet<QString> referencedExternals;
};

BindResult bindProgram(const ParseResult& program,
                       const QSet<QString>& knownExternals = QSet<QString>());

// Word list for code completion: keywords, declared symbols, and unresolved
// references (typically native functions/properties) seen in the program.
QStringList completionEntries(const ParseResult& program);

} // namespace ObScript
