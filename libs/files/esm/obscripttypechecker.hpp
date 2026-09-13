#pragma once

#include <QString>
#include <QVector>
#include <QHash>

#include "obscriptparser.hpp"

// Static type checking for OBScript against a game-specific catalog of
// native functions and properties. The catalog is supplied by the caller
// (each game ships a different set), so the checker stays game-agnostic.
namespace ObScript
{

enum class ValueType
{
    Unknown,
    Any,
    Int,
    Float,
    String,
    Bool,
    Nil,
    Form,
    Object
};

QString valueTypeName(ValueType t);

struct NativeParam
{
    QString name;
    ValueType type = ValueType::Any;
    bool optional = false;
};

struct NativeFunction
{
    QString name;
    QVector<NativeParam> params;
    ValueType returnType = ValueType::Any;
    bool isProperty = false; // true = read-only property, no call
};

/// Catalog of known native functions/properties for a game.
class NativeCatalog
{
public:
    void add(const NativeFunction& fn) { m_functions.insert(fn.name.toLower(), fn); }
    bool contains(const QString& name) const { return m_functions.contains(name.toLower()); }
    const NativeFunction* find(const QString& name) const
    {
        auto it = m_functions.constFind(name.toLower());
        return it != m_functions.constEnd() ? &it.value() : nullptr;
    }
    const QHash<QString, NativeFunction>& all() const { return m_functions; }

private:
    QHash<QString, NativeFunction> m_functions;
};

enum class TypeSeverity
{
    Warning,
    Error
};

struct TypeDiagnostic
{
    TypeSeverity severity = TypeSeverity::Error;
    QString message;
    int line = 1;
};

struct TypeCheckResult
{
    bool ok = true;
    QVector<TypeDiagnostic> diagnostics;
};

/// Type-checks a parsed program against a native catalog. Reports argument
/// count/type mismatches on calls to known natives and obvious literal
/// type errors. Calls to unknown functions are reported as warnings (they
/// may be user-defined or from an unlisted game).
TypeCheckResult typeCheck(const ParseResult& program,
                          const NativeCatalog& catalog);

/// Builds a minimal catalog of the functions/properties common to the
/// Bethesda script families. Games may extend or replace it.
NativeCatalog builtinCatalog();

} // namespace ObScript
