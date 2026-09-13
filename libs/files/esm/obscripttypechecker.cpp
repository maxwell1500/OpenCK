#include "obscripttypechecker.hpp"

namespace ObScript
{

QString valueTypeName(ValueType t)
{
    switch (t)
    {
    case ValueType::Unknown: return QStringLiteral("unknown");
    case ValueType::Any: return QStringLiteral("any");
    case ValueType::Int: return QStringLiteral("int");
    case ValueType::Float: return QStringLiteral("float");
    case ValueType::String: return QStringLiteral("string");
    case ValueType::Bool: return QStringLiteral("bool");
    case ValueType::Nil: return QStringLiteral("nil");
    case ValueType::Form: return QStringLiteral("form");
    case ValueType::Object: return QStringLiteral("object");
    }
    return QStringLiteral("unknown");
}

namespace
{

// Numeric compatibility: int and float are mutually assignable.
bool isAssignable(ValueType expected, ValueType actual)
{
    if (expected == ValueType::Any || expected == ValueType::Unknown)
        return true;
    if (actual == ValueType::Unknown || actual == ValueType::Any)
        return true; // avoid cascading errors
    if (expected == actual)
        return true;
    if ((expected == ValueType::Int || expected == ValueType::Float)
        && (actual == ValueType::Int || actual == ValueType::Float))
        return true;
    if (expected == ValueType::Bool && (actual == ValueType::Int))
        return true;
    if (expected == ValueType::Int && actual == ValueType::Bool)
        return true;
    if (expected == ValueType::Object && actual == ValueType::Form)
        return true;
    return false;
}

class TypeChecker
{
public:
    TypeChecker(const NativeCatalog& catalog) : m_catalog(catalog) {}

    TypeCheckResult run(const ParseResult& program)
    {
        for (const auto& stmt : program.statements)
            checkStmt(*stmt);
        return m_result;
    }

private:
    void report(TypeSeverity sev, const QString& msg, int line)
    {
        TypeDiagnostic d;
        d.severity = sev;
        d.message = msg;
        d.line = line;
        m_result.diagnostics.append(d);
        if (sev == TypeSeverity::Error)
            m_result.ok = false;
    }

    void checkBlock(const std::vector<StmtPtr>& block)
    {
        for (const auto& stmt : block)
            checkStmt(*stmt);
    }

    void checkStmt(const Statement& stmt)
    {
        switch (stmt.kind)
        {
        case StmtKind::If:
            if (stmt.condition)
                inferType(*stmt.condition);
            checkBlock(stmt.body);
            for (auto& branch : stmt.branches)
            {
                if (branch.first)
                    inferType(*branch.first);
                checkBlock(branch.second);
            }
            checkBlock(stmt.elseBody);
            break;

        case StmtKind::While:
            if (stmt.condition)
                inferType(*stmt.condition);
            checkBlock(stmt.body);
            break;

        case StmtKind::For:
            if (stmt.forInit) inferType(*stmt.forInit);
            if (stmt.forEnd) inferType(*stmt.forEnd);
            checkBlock(stmt.body);
            break;

        case StmtKind::Return:
            if (stmt.value)
                inferType(*stmt.value);
            break;

        case StmtKind::Let:
            if (stmt.value)
                inferType(*stmt.value);
            break;

        case StmtKind::ExprStmt:
            if (stmt.value)
                inferType(*stmt.value);
            break;

        case StmtKind::Function:
            checkBlock(stmt.body);
            break;
        }
    }

    ValueType inferType(const Expr& expr)
    {
        switch (expr.kind)
        {
        case ExprKind::LiteralInt: return ValueType::Int;
        case ExprKind::LiteralFloat: return ValueType::Float;
        case ExprKind::LiteralString: return ValueType::String;
        case ExprKind::LiteralBool: return ValueType::Bool;
        case ExprKind::LiteralNil: return ValueType::Nil;
        case ExprKind::Identifier:
        {
            const NativeFunction* fn = m_catalog.find(expr.name);
            if (fn && fn->isProperty)
                return fn->returnType;
            return ValueType::Unknown;
        }

        case ExprKind::UnaryOp:
        {
            inferType(*expr.operand);
            if (expr.op == "!") return ValueType::Bool;
            return ValueType::Unknown;
        }

        case ExprKind::BinaryOp:
        {
            const ValueType l = inferType(*expr.left);
            const ValueType r = inferType(*expr.right);
            if (expr.op == "==" || expr.op == "!=" || expr.op == "<"
                || expr.op == "<=" || expr.op == ">" || expr.op == ">="
                || expr.op == "&&" || expr.op == "||")
                return ValueType::Bool;
            if (expr.op == "+" && (l == ValueType::String || r == ValueType::String))
                return ValueType::String;
            if (l == ValueType::Float || r == ValueType::Float)
                return ValueType::Float;
            return ValueType::Int;
        }

        case ExprKind::Call:
            return checkCall(expr);

        case ExprKind::FieldAccess:
            if (expr.base)
                inferType(*expr.base);
            return ValueType::Unknown;

        case ExprKind::Index:
            if (expr.base) inferType(*expr.base);
            if (expr.index) inferType(*expr.index);
            return ValueType::Unknown;
        }
        return ValueType::Unknown;
    }

    ValueType checkCall(const Expr& expr)
    {
        QString calleeName;
        if (expr.callee && expr.callee->kind == ExprKind::Identifier)
            calleeName = expr.callee->name;

        // Infer argument types (also validates nested calls)
        QVector<ValueType> argTypes;
        for (const auto& arg : expr.args)
            argTypes.append(inferType(*arg));

        if (calleeName.isEmpty())
            return ValueType::Unknown;

        const NativeFunction* fn = m_catalog.find(calleeName);
        if (!fn)
        {
            report(TypeSeverity::Warning,
                   QStringLiteral("unknown function '%1'").arg(calleeName),
                   expr.line);
            return ValueType::Unknown;
        }

        if (fn->isProperty)
        {
            report(TypeSeverity::Error,
                   QStringLiteral("'%1' is a property and cannot be called")
                       .arg(calleeName),
                   expr.line);
            return fn->returnType;
        }

        const int required = [&]() {
            int n = 0;
            for (const auto& p : fn->params)
                if (!p.optional) ++n;
            return n;
        }();

        if (static_cast<int>(expr.args.size()) < required
            || static_cast<int>(expr.args.size()) > fn->params.size())
        {
            report(TypeSeverity::Error,
                   QStringLiteral("'%1' expects %2 argument(s), got %3")
                       .arg(calleeName)
                       .arg(fn->params.size())
                       .arg(expr.args.size()),
                   expr.line);
            return fn->returnType;
        }

        for (int i = 0; i < argTypes.size() && i < fn->params.size(); ++i)
        {
            if (!isAssignable(fn->params[i].type, argTypes[i]))
            {
                report(TypeSeverity::Error,
                       QStringLiteral("argument %1 of '%2' expects %3, got %4")
                           .arg(i + 1)
                           .arg(calleeName)
                           .arg(valueTypeName(fn->params[i].type))
                           .arg(valueTypeName(argTypes[i])),
                       expr.line);
            }
        }

        return fn->returnType;
    }

    const NativeCatalog& m_catalog;
    TypeCheckResult m_result;
};

} // namespace

TypeCheckResult typeCheck(const ParseResult& program,
                          const NativeCatalog& catalog)
{
    TypeChecker checker(catalog);
    return checker.run(program);
}

NativeCatalog builtinCatalog()
{
    NativeCatalog catalog;

    auto add = [&](const QString& name, QVector<NativeParam> params,
                   ValueType ret) {
        NativeFunction fn;
        fn.name = name;
        fn.params = params;
        fn.returnType = ret;
        catalog.add(fn);
    };
    auto addProp = [&](const QString& name, ValueType ret) {
        NativeFunction fn;
        fn.name = name;
        fn.returnType = ret;
        fn.isProperty = true;
        catalog.add(fn);
    };

    add(QStringLiteral("print"), {{QStringLiteral("text"), ValueType::String, false}},
        ValueType::Nil);
    add(QStringLiteral("messagebox"),
        {{QStringLiteral("text"), ValueType::String, false}}, ValueType::Nil);
    add(QStringLiteral("abs"),
        {{QStringLiteral("value"), ValueType::Float, false}}, ValueType::Float);
    add(QStringLiteral("rnd"),
        {{QStringLiteral("max"), ValueType::Int, false}}, ValueType::Int);
    add(QStringLiteral("getdistance"),
        {{QStringLiteral("target"), ValueType::Object, false}}, ValueType::Float);
    add(QStringLiteral("getitemcount"),
        {{QStringLiteral("item"), ValueType::Form, false}}, ValueType::Int);
    add(QStringLiteral("removeitem"),
        {{QStringLiteral("item"), ValueType::Form, false},
         {QStringLiteral("count"), ValueType::Int, false}},
        ValueType::Nil);
    add(QStringLiteral("additem"),
        {{QStringLiteral("item"), ValueType::Form, false},
         {QStringLiteral("count"), ValueType::Int, false}},
        ValueType::Nil);
    add(QStringLiteral("playgroup"),
        {{QStringLiteral("group"), ValueType::String, false},
         {QStringLiteral("flags"), ValueType::Int, true}},
        ValueType::Nil);
    add(QStringLiteral("getactorvalue"),
        {{QStringLiteral("value"), ValueType::String, false}}, ValueType::Float);

    addProp(QStringLiteral("player"), ValueType::Object);
    addProp(QStringLiteral("self"), ValueType::Object);
    addProp(QStringLiteral("health"), ValueType::Float);
    addProp(QStringLiteral("magicka"), ValueType::Float);
    addProp(QStringLiteral("fatigue"), ValueType::Float);

    return catalog;
}

} // namespace ObScript
