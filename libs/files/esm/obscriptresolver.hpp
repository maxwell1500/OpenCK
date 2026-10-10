#pragma once

#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

#include "obscriptparser.hpp"

// Cross-script resolution for OBScript/Papyrus: checks that a script's
// properties, function calls and local declarations reference scripts that
// exist in the plugin's own script set. Unlike native-catalog type checking,
// this pass needs no game data — it only needs the set of script names the
// loaded plugin already knows about.
namespace ObScript
{

/// Script visibility: one entry per script in the plugin (SCRO records).
struct ScriptInfo
{
    ScriptInfo() = default;
    ScriptInfo(const QString& name_, const QStringList& functions_)
        : name(name_), functionNames(functions_) {}
    QString name;
    QStringList functionNames; ///< functions this script exports, incl. inherited
};

enum class ResolveSeverity
{
    Warning,
    Error
};

struct ResolveDiagnostic
{
    ResolveSeverity severity = ResolveSeverity::Error;
    QString message;
    int line = 1;
    /// The script name involved, when applicable.
    QString symbol;
};

struct ResolveResult
{
    bool ok = true;
    QVector<ResolveDiagnostic> diagnostics;
    /// Property scripts referenced but absent from the script set.
    QSet<QString> unresolvedScripts;
};

/// Resolves a parsed program against the plugin's script set.
/// `knownScripts` is the loaded plugin's SCRO editor ids.
ResolveResult resolveProgram(const ParseResult& program,
                             const QVector<ScriptInfo>& knownScripts);

/// Script names referenced as property types, for error reporting.
QStringList referencedScriptNames(const ParseResult& program);

} // namespace ObScript
