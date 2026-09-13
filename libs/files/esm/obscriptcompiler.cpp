#include "obscriptcompiler.hpp"

namespace ObScript
{

namespace
{

class CodeGen
{
public:
    void compileProgram(const ParseResult& program);
    BytecodeProgram takeProgram() { return m_prog; }

private:
    int emitInstr(Opcode op)
    {
        m_prog.code.append(Instr{op});
        return m_prog.code.size() - 1;
    }

    Instr& lastInstr() { return m_prog.code.last(); }

    int symbolIndex(const QString& name)
    {
        auto it = m_symbols.constFind(name);
        if (it != m_symbols.constEnd())
            return it.value();
        const int index = m_nextSymbol++;
        m_symbols.insert(name, index);
        m_prog.symbolNames.append(name);
        return index;
    }

    void compileBlock(const std::vector<StmtPtr>& block)
    {
        for (const auto& stmt : block)
            compileStmt(*stmt);
    }

    void compileStmt(const Statement& stmt);
    void compileExpr(const Expr& expr);

    BytecodeProgram m_prog;
    QHash<QString, int> m_symbols;
    int m_nextSymbol = 0;
};

void CodeGen::compileProgram(const ParseResult& program)
{
    compileBlock(program.statements);
    emitInstr(Opcode::Halt);
}

void CodeGen::compileStmt(const Statement& stmt)
{
    switch (stmt.kind)
    {
    case StmtKind::If:
    {
        compileExpr(*stmt.condition);
        const int jzIdx = emitInstr(Opcode::Jz);
        compileBlock(stmt.body);

        if (!stmt.elseBody.empty())
        {
            const int elseJmpIdx = emitInstr(Opcode::Jmp);
            const int elseTarget = m_prog.code.size();
            compileBlock(stmt.elseBody);
            const int endTarget = m_prog.code.size();
            m_prog.code[jzIdx].operand = static_cast<quint32>(elseTarget);
            m_prog.code[elseJmpIdx].operand = static_cast<quint32>(endTarget);
        }
        else
        {
            m_prog.code[jzIdx].operand = static_cast<quint32>(m_prog.code.size());
        }
        break;
    }

    case StmtKind::While:
    {
        const int whileStart = m_prog.code.size();
        compileExpr(*stmt.condition);
        const int jzIdx = emitInstr(Opcode::Jz);
        compileBlock(stmt.body);
        const int jmpIdx = emitInstr(Opcode::Jmp);
        m_prog.code[jmpIdx].operand = static_cast<quint32>(whileStart);
        m_prog.code[jzIdx].operand = static_cast<quint32>(m_prog.code.size());
        break;
    }

    case StmtKind::For:
    {
        if (stmt.forInit)
            compileExpr(*stmt.forInit);
        const int loopStart = m_prog.code.size();
        if (stmt.condition)
            compileExpr(*stmt.condition);
        const int jzIdx = emitInstr(Opcode::Jz);
        compileBlock(stmt.body);
        const int jmpIdx = emitInstr(Opcode::Jmp);
        m_prog.code[jmpIdx].operand = static_cast<quint32>(loopStart);
        m_prog.code[jzIdx].operand = static_cast<quint32>(m_prog.code.size());
        break;
    }

    case StmtKind::Return:
        if (stmt.value)
        {
            compileExpr(*stmt.value);
            emitInstr(Opcode::RetVal);
        }
        else
        {
            emitInstr(Opcode::Ret);
        }
        break;

    case StmtKind::Let:
    {
        if (stmt.value)
            compileExpr(*stmt.value);
        if (stmt.lhs && stmt.lhs->kind == ExprKind::Identifier)
        {
            const int index = symbolIndex(stmt.lhs->name);
            const int storeIdx = emitInstr(Opcode::StoreVar);
            m_prog.code[storeIdx].operand = static_cast<quint32>(index);
        }
        break;
    }

    case StmtKind::ExprStmt:
        if (stmt.value)
            compileExpr(*stmt.value);
        break;

    case StmtKind::Function:
        break;
    }
}

void CodeGen::compileExpr(const Expr& expr)
{
    switch (expr.kind)
    {
    case ExprKind::Identifier:
    {
        const int index = symbolIndex(expr.name);
        const int loadIdx = emitInstr(Opcode::LoadVar);
        m_prog.code[loadIdx].operand = static_cast<quint32>(index);
        break;
    }

    case ExprKind::LiteralInt:
    {
        const int idx = emitInstr(Opcode::PushInt);
        m_prog.code[idx].intValue = expr.intValue;
        break;
    }

    case ExprKind::LiteralFloat:
    {
        const int idx = emitInstr(Opcode::PushFloat);
        m_prog.code[idx].floatValue = expr.floatValue;
        break;
    }

    case ExprKind::LiteralString:
    {
        const int idx = emitInstr(Opcode::PushString);
        m_prog.code[idx].stringValue = expr.stringValue;
        break;
    }

    case ExprKind::LiteralBool:
    {
        const int idx = emitInstr(Opcode::PushBool);
        m_prog.code[idx].intValue = expr.boolValue ? 1 : 0;
        break;
    }

    case ExprKind::LiteralNil:
        emitInstr(Opcode::PushNil);
        break;

    case ExprKind::BinaryOp:
        compileExpr(*expr.left);
        compileExpr(*expr.right);
        if (expr.op == "+") emitInstr(Opcode::Add);
        else if (expr.op == "-") emitInstr(Opcode::Sub);
        else if (expr.op == "*") emitInstr(Opcode::Mul);
        else if (expr.op == "/") emitInstr(Opcode::Div);
        else if (expr.op == "%") emitInstr(Opcode::Mod);
        else if (expr.op == "==") emitInstr(Opcode::Eq);
        else if (expr.op == "!=") emitInstr(Opcode::Neq);
        else if (expr.op == "<") emitInstr(Opcode::Lt);
        else if (expr.op == "<=") emitInstr(Opcode::Leq);
        else if (expr.op == ">") emitInstr(Opcode::Gt);
        else if (expr.op == ">=") emitInstr(Opcode::Geq);
        else if (expr.op == "&&") emitInstr(Opcode::And);
        else if (expr.op == "||") emitInstr(Opcode::Or);
        else if (expr.op == "&") emitInstr(Opcode::BitAnd);
        else if (expr.op == "|") emitInstr(Opcode::BitOr);
        break;

    case ExprKind::UnaryOp:
        compileExpr(*expr.operand);
        if (expr.op == "-") emitInstr(Opcode::Neg);
        else if (expr.op == "!") emitInstr(Opcode::Not);
        else if (expr.op == "~") emitInstr(Opcode::BitNot);
        break;

    case ExprKind::Call:
    {
        compileExpr(*expr.callee);
        for (const auto& arg : expr.args)
            compileExpr(*arg);
        const int callIdx = emitInstr(Opcode::Call);
        m_prog.code[callIdx].operand = static_cast<quint32>(expr.args.size());
        break;
    }

    default:
        break;
    }
}

} // namespace

BytecodeProgram compile(const ParseResult& program)
{
    CodeGen cg;
    cg.compileProgram(program);
    BytecodeProgram prog = cg.takeProgram();
    prog.ok = true;
    return prog;
}

} // namespace ObScript
