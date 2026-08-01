#ifndef PAPYRUSTYPECHECKER_HPP
#define PAPYRUSTYPECHECKER_HPP

#include <QString>
#include <QVector>
#include <QMap>
#include <QSet>

class PapyrusTypeChecker
{
public:
    struct TypeInfo
    {
        QString name;
        bool isArray = false;
        bool isScriptType = false;

        bool isValid() const { return !name.isEmpty(); }
        bool isBuiltin() const;
        bool isNumeric() const { return name == "Int" || name == "Float"; }
        bool isBool() const { return name == "Bool"; }
        bool isString() const { return name == "String"; }

        bool operator==(const TypeInfo& other) const
        {
            return name == other.name && isArray == other.isArray;
        }
        bool operator!=(const TypeInfo& other) const { return !(*this == other); }
    };

    struct FunctionSignature
    {
        QString name;
        QVector<TypeInfo> paramTypes;
        TypeInfo returnType;
        int line = 0;
    };

    struct TypeError
    {
        int line;
        QString message;
    };

    PapyrusTypeChecker();

    void setScriptName(const QString& name);
    void declareVariable(const QString& name, const TypeInfo& type, int line = 0);
    void declareFunction(const FunctionSignature& sig);
    void registerScriptType(const QString& name);

    // Registers a member (property) type for a script type, e.g.
    // registerProperty("Actor", "IsInCombat", TypeInfo{Bool}) lets
    // `actor.IsInCombat` resolve to Bool. Also declares the implicit
    // ".length" member for arrays.
    void registerProperty(const QString& scriptType, const QString& property,
                          const TypeInfo& type);
    bool hasProperty(const QString& scriptType, const QString& property) const;
    TypeInfo propertyType(const QString& scriptType, const QString& property) const;

    // Resolves the type of a member access expression "obj.Property".
    // Returns an invalid TypeInfo if obj is not a declared variable or the
    // property is unknown.
    TypeInfo resolveMemberAccess(const QString& objectExpr,
                                 const QString& member) const;

    // Resolves "Array.Length" to Int when arrayType is an array.
    TypeInfo resolveArrayLength(const TypeInfo& arrayType) const;

    bool hasVariable(const QString& name) const;
    TypeInfo variableType(const QString& name) const;
    bool hasFunction(const QString& name) const;

    bool checkAssignment(const QString& varName, const TypeInfo& valueType, int line = 0);
    bool checkFunctionCall(const QString& funcName, const QVector<TypeInfo>& argTypes, int line = 0);
    bool checkConditional(const TypeInfo& exprType, int line = 0, const QString& keyword = "If");
    bool checkArrayIndex(const TypeInfo& arrayType, int line = 0);
    bool checkArrayElement(const TypeInfo& arrayType, const TypeInfo& elementType, int line = 0);

    bool isAssignableFrom(const TypeInfo& target, const TypeInfo& source) const;
    TypeInfo elementTypeOf(const TypeInfo& arrayType) const;
    TypeInfo inferLiteralType(const QString& literal) const;
    TypeInfo inferExpressionType(const QString& expr) const;

    const QMap<QString, TypeInfo>& symbols() const { return m_symbols; }
    const QMap<QString, FunctionSignature>& functions() const { return m_functions; }
    QVector<QString> getErrors() const;
    QVector<TypeError> typedErrors() const { return m_errors; }
    void clearErrors() { m_errors.clear(); }
    void clear();

    static bool isBuiltinType(const QString& name);
    static bool isBuiltinArrayType(const QString& name);
    static TypeInfo parseType(const QString& typeText);

private:
    bool compatible(const TypeInfo& target, const TypeInfo& source) const;
    void addError(int line, const QString& message);

    QMap<QString, TypeInfo> m_symbols;
    QMap<QString, FunctionSignature> m_functions;
    QMap<QString, QMap<QString, TypeInfo>> m_properties; // scriptType -> property -> type
    QVector<TypeError> m_errors;
    QString m_scriptName;
    QSet<QString> m_scriptTypes;
};

#endif // PAPYRUSTYPECHECKER_HPP