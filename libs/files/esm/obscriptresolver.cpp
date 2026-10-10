#include "obscriptresolver.hpp"

namespace ObScript
{

namespace
{

// Built-in script types that exist in every Creation Engine game, so a
// property of these types never needs a matching SCRO record.
const QSet<QString>& builtinScriptTypes()
{
    static const QSet<QString> k = {
        QStringLiteral("scriptobject"),
        QStringLiteral("form"),
        QStringLiteral("objectreference"),
        QStringLiteral("actor"),
        QStringLiteral("actorbase"),
        QStringLiteral("topic"),
        QStringLiteral("imagespacemodifier"),
        QStringLiteral("impactdataset"),
        QStringLiteral("musictype"),
        // primitive-valued property types
        QStringLiteral("int"),
        QStringLiteral("float"),
        QStringLiteral("bool"),
        QStringLiteral("string"),
        QStringLiteral("keyword"),
        QStringLiteral("location"),
        QStringLiteral("referencealias"),
        QStringLiteral("alias"),
        QStringLiteral("activemagiceffect"),
        QStringLiteral("visualeffect"),
        QStringLiteral("armoraddon"),
    };
    return k;
}

bool isBuiltinType(const QString& name)
{
    if (name.isEmpty())
    {
        return true;
    }
    // Keyword tokens are lower-cased by the lexer, so `int Property count`
    // arrives here as "int"; compare case-insensitively and the primitive
    // types resolve as well.
    return builtinScriptTypes().contains(name.toLower());
}

// Lower-cased set for case-insensitive script name comparison. OBScript
// identifiers are case-insensitive at the compiler, so the lookup must be too.
QHash<QString, ScriptInfo> indexByName(const QVector<ScriptInfo>& knownScripts)
{
    QHash<QString, ScriptInfo> idx;
    for (const ScriptInfo& s : knownScripts)
    {
        idx.insert(s.name.toLower(), s);
    }
    return idx;
}

} // namespace

QStringList referencedScriptNames(const ParseResult& program)
{
    QStringList out;
    for (const ObScript::PropertyDecl& p : propertyDecls(program))
    {
        out.append(p.typeName);
    }
    return out;
}

ResolveResult resolveProgram(const ParseResult& program,
                             const QVector<ScriptInfo>& knownScripts)
{
    ResolveResult result;
    const QHash<QString, ScriptInfo> index = indexByName(knownScripts);

    for (const ObScript::PropertyDecl& p : propertyDecls(program))
    {
        const QString typeName = p.typeName.trimmed();
        if (typeName.isEmpty() || isBuiltinType(typeName))
        {
            continue;
        }
        if (!index.contains(typeName.toLower()))
        {
            ResolveDiagnostic d;
            d.severity = ResolveSeverity::Warning;
            d.message = QStringLiteral("property '%1' references script '%2', "
                                       "which is not present in this plugin")
                            .arg(p.name, typeName);
            d.line = p.line;
            d.symbol = typeName;
            result.diagnostics.append(d);
            result.unresolvedScripts.insert(typeName);
        }
    }

    // Unresolved-property diagnostics are warnings: the script may come from
    // a master file the caller did not pass in, so they never fail `ok`.
    return result;
}

} // namespace ObScript
