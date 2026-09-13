#include "obscriptbinder.hpp"

#include "obscriptlexer.hpp"

#include <QHash>
#include <QSet>

namespace ObScript
{

namespace
{

class Binder
{
public:
    explicit Binder(const QSet<QString>& knownExternals)
        : m_knownExternals(knownExternals)
    {
    }

    BindResult run(const ParseResult& program)
    {
        // Pass 1: register functions and top-level globals so that references
        // are order-independent.
        for (const StmtPtr& stmt : program.statements)
        {
            if (!stmt)
            {
                continue;
            }
            if (stmt->kind == StmtKind::Function)
            {
                registerFunction(*stmt);
            }
            else if (stmt->kind == StmtKind::Let && stmt->declares && stmt->lhs &&
                     stmt->lhs->kind == ExprKind::Identifier)
            {
                registerGlobal(stmt->lhs->name, stmt->lhs->line);
            }
        }

        // Pass 2: resolve references.
        for (const StmtPtr& stmt : program.statements)
        {
            if (!stmt)
            {
                continue;
            }
            if (stmt->kind == StmtKind::Function)
            {
                bindFunction(*stmt);
            }
            else
            {
                m_locals.clear();
                bindStatement(*stmt);
            }
        }

        for (const BindDiagnostic& d : m_result.diagnostics)
        {
            if (d.severity == BindSeverity::Error)
            {
                m_result.ok = false;
                break;
            }
        }
        return m_result;
    }

private:
    void error(const QString& message, int line)
    {
        m_result.diagnostics.append({BindSeverity::Error, message, line});
    }

    void warning(const QString& message, int line)
    {
        m_result.diagnostics.append({BindSeverity::Warning, message, line});
    }

    void registerFunction(const Statement& fn)
    {
        if (m_functions.contains(fn.funcName))
        {
            error(QStringLiteral("duplicate function '%1'").arg(fn.funcName), fn.line);
            return;
        }
        m_functions.insert(fn.funcName);
        m_functionParamCount.insert(fn.funcName, int(fn.params.size()));
        m_result.symbols.append(
            {fn.funcName, BindSymbolKind::Function, fn.line, int(fn.params.size())});
    }

    void registerGlobal(const QString& name, int line)
    {
        if (m_globals.contains(name))
        {
            warning(QStringLiteral("global '%1' declared more than once").arg(name), line);
            return;
        }
        m_globals.insert(name);
        m_result.symbols.append({name, BindSymbolKind::Global, line, 0});
    }

    void declareLocal(const QString& name, int line, BindSymbolKind kind)
    {
        if (m_locals.contains(name))
        {
            error(QStringLiteral("duplicate declaration of '%1'").arg(name), line);
            return;
        }
        if (kind == BindSymbolKind::Parameter && m_globals.contains(name))
        {
            warning(QStringLiteral("parameter '%1' shadows a global").arg(name), line);
        }
        m_locals.insert(name);
        m_result.symbols.append({name, kind, line, 0});
    }

    void bindFunction(const Statement& fn)
    {
        m_locals.clear();
        for (const QString& p : fn.params)
        {
            declareLocal(p, fn.line, BindSymbolKind::Parameter);
        }
        for (const StmtPtr& s : fn.body)
        {
            if (s)
            {
                bindStatement(*s);
            }
        }
        m_locals.clear();
    }

    void bindBlock(const std::vector<StmtPtr>& body)
    {
        for (const StmtPtr& s : body)
        {
            if (s)
            {
                bindStatement(*s);
            }
        }
    }

    void bindStatement(const Statement& s)
    {
        switch (s.kind)
        {
        case StmtKind::If:
            bindExpression(s.condition.get());
            bindBlock(s.body);
            for (const auto& branch : s.branches)
            {
                bindExpression(branch.first.get());
                bindBlock(branch.second);
            }
            bindBlock(s.elseBody);
            break;
        case StmtKind::While:
            bindExpression(s.condition.get());
            bindBlock(s.body);
            break;
        case StmtKind::For:
            if (!s.forVar.isEmpty())
            {
                declareLocal(s.forVar, s.line, BindSymbolKind::Local);
            }
            bindExpression(s.forInit.get());
            bindExpression(s.forEnd.get());
            bindBlock(s.body);
            break;
        case StmtKind::Return:
            bindExpression(s.value.get());
            break;
        case StmtKind::Let:
            if (s.declares && s.lhs && s.lhs->kind == ExprKind::Identifier)
            {
                declareLocal(s.lhs->name, s.lhs->line, BindSymbolKind::Local);
            }
            else
            {
                bindExpression(s.lhs.get());
            }
            bindExpression(s.value.get());
            break;
        case StmtKind::ExprStmt:
            bindExpression(s.value.get());
            break;
        case StmtKind::Function:
            bindFunction(s);
            break;
        }
    }

    void reference(const QString& name, int line)
    {
        if (m_locals.contains(name) || m_globals.contains(name) ||
            m_functions.contains(name) || m_knownExternals.contains(name))
        {
            return;
        }
        m_result.referencedExternals.insert(name);
        (void)line;
    }

    void bindExpression(const Expr* e)
    {
        if (!e)
        {
            return;
        }
        switch (e->kind)
        {
        case ExprKind::Identifier:
            reference(e->name, e->line);
            break;
        case ExprKind::LiteralInt:
        case ExprKind::LiteralFloat:
        case ExprKind::LiteralString:
        case ExprKind::LiteralBool:
        case ExprKind::LiteralNil:
            break;
        case ExprKind::BinaryOp:
            bindExpression(e->left.get());
            bindExpression(e->right.get());
            break;
        case ExprKind::UnaryOp:
            bindExpression(e->operand.get());
            break;
        case ExprKind::Call:
            bindExpression(e->callee.get());
            for (const ExprPtr& a : e->args)
            {
                bindExpression(a.get());
            }
            break;
        case ExprKind::FieldAccess:
            bindExpression(e->base.get());
            break;
        case ExprKind::Index:
            bindExpression(e->base.get());
            bindExpression(e->index.get());
            break;
        }
    }

    QSet<QString> m_knownExternals;
    QSet<QString> m_functions;
    QHash<QString, int> m_functionParamCount;
    QSet<QString> m_globals;
    QSet<QString> m_locals;
    BindResult m_result;
};

} // namespace

BindResult bindProgram(const ParseResult& program, const QSet<QString>& knownExternals)
{
    return Binder(knownExternals).run(program);
}

QStringList completionEntries(const ParseResult& program)
{
    QSet<QString> words;
    for (const QString& k : keywords())
    {
        words.insert(k);
    }
    BindResult bound = bindProgram(program);
    for (const BindSymbol& s : bound.symbols)
    {
        words.insert(s.name);
    }
    for (const QString& e : bound.referencedExternals)
    {
        words.insert(e);
    }
    QStringList out = words.values();
    out.sort();
    return out;
}

} // namespace ObScript
