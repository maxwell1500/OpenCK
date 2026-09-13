#pragma once

#include <QString>
#include <QVector>
#include <memory>
#include <utility>
#include <vector>

#include "obscriptlexer.hpp"

// Syntax parser for the OBScript subset produced by ObScript::Lexer. It builds
// a small AST (expressions with full operator precedence plus the common
// control-flow statements) and reports the first syntax error with a line
// number. It is deliberately a practical subset: switch/case, try/catch,
// property/event/local declarations are not supported and produce a clear
// error. No semantics or symbol binding are performed here.
namespace ObScript
{

struct Expr;
struct Statement;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Statement>;

enum class ExprKind
{
    Identifier,
    LiteralInt,
    LiteralFloat,
    LiteralString,
    LiteralBool,
    LiteralNil,
    BinaryOp,
    UnaryOp,
    Call,
    FieldAccess,
    Index
};

struct Expr
{
    ExprKind kind = ExprKind::Identifier;
    QString name; // Identifier
    qint64 intValue = 0;
    double floatValue = 0.0;
    QString stringValue;
    bool boolValue = false;
    QString op; // BinaryOp / UnaryOp
    ExprPtr left;
    ExprPtr right;
    ExprPtr operand; // UnaryOp
    ExprPtr callee; // Call
    std::vector<ExprPtr> args;
    ExprPtr base; // FieldAccess / Index
    ExprPtr index; // Index
    int line = 1;
};

enum class StmtKind
{
    If,
    While,
    For,
    Return,
    Let,
    ExprStmt,
    Function
};

struct Statement
{
    StmtKind kind = StmtKind::ExprStmt;
    ExprPtr condition; // If / While
    std::vector<StmtPtr> body; // If-then / While / For / Function
    std::vector<std::pair<ExprPtr, std::vector<StmtPtr>>> branches; // If elseifs
    std::vector<StmtPtr> elseBody; // If
    QString forVar;
    ExprPtr forInit;
    ExprPtr forEnd; // For
    ExprPtr lhs; // Let / (assignment)
    ExprPtr value; // Let / Return / ExprStmt
    bool declares = false; // Let: true for `let`, false for `set`/bare assignment
    QString funcName; // Function
    QVector<QString> params; // Function
    int line = 1;
};

struct ParseResult
{
    bool ok = false;
    QString error;
    int errorLine = 0;
    std::vector<StmtPtr> statements;
};

class Parser
{
public:
    explicit Parser(const QString& source);
    ParseResult parse();

private:
    const Token& peek() const;
    const Token& peekAt(int off) const;
    Token advance();
    bool atEnd() const;

    bool isTerminator(const QStringList& terms) const;
    bool expectOperator(const QString& op);
    bool expectKeyword(const QString& kw);
    Token expectIdentifier();
    void fail(const QString& msg, int line);

    std::vector<StmtPtr> parseBlock(const QStringList& terminators);
    StmtPtr parseStatement();
    StmtPtr parseIf();
    StmtPtr parseWhile();
    StmtPtr parseFor();
    StmtPtr parseReturn();
    StmtPtr parseLet(bool declares);
    StmtPtr parseFunction();

    ExprPtr parseExpression();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseEquality();
    ExprPtr parseRelational();
    ExprPtr parseAdditive();
    ExprPtr parseMultiplicative();
    ExprPtr parseUnary();
    ExprPtr parsePostfix();
    ExprPtr parsePrimary();

    QVector<Token> m_tokens;
    int m_pos = 0;
    bool m_error = false;
    QString m_errorMsg;
    int m_errorLine = 0;
};

ParseResult parse(const QString& source);

} // namespace ObScript
