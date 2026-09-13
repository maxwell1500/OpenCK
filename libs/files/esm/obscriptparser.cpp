#include "obscriptparser.hpp"

#include <QSet>

namespace ObScript
{

namespace
{

bool isTypeKeyword(const QString& kw)
{
    static const QSet<QString> k = {
        "float", "int", "bool", "string", "objectreference", "keyword",
        "actorvalue", "form", "formlist", "package", "quest", "sound",
        "spell", "magiceffect", "script", "ref", "reference"
    };
    return k.contains(kw);
}

} // namespace

Parser::Parser(const QString& source)
{
    Lexer lexer(source);
    m_tokens = lexer.tokenizeAll();
    if (lexer.hasError())
    {
        m_error = true;
        m_errorMsg = lexer.errorMessage();
        m_errorLine = m_tokens.isEmpty() ? 1 : m_tokens.last().line;
    }
    m_tokens.append(Token{TokenKind::End, QString(), m_tokens.isEmpty() ? 1
                                                                     : m_tokens.last().line});
}

const Token& Parser::peek() const
{
    return m_tokens.at(qMin(m_pos, m_tokens.size() - 1));
}

const Token& Parser::peekAt(int off) const
{
    const int i = qMin(m_pos + off, m_tokens.size() - 1);
    return m_tokens.at(i);
}

Token Parser::advance()
{
    Token t = peek();
    if (m_pos < m_tokens.size() - 1)
    {
        ++m_pos;
    }
    return t;
}

bool Parser::atEnd() const
{
    return peek().kind == TokenKind::End;
}

bool Parser::isTerminator(const QStringList& terms) const
{
    const Token& t = peek();
    if (t.kind != TokenKind::Keyword)
    {
        return false;
    }
    return terms.contains(t.text);
}

bool Parser::expectOperator(const QString& op)
{
    const Token& t = peek();
    if (t.kind == TokenKind::Operator && t.text == op)
    {
        advance();
        return true;
    }
    fail(QStringLiteral("expected '%1' but found '%2'")
               .arg(op, t.kind == TokenKind::End ? QString("end of script") : t.text)
               ,
         t.line);
    return false;
}

bool Parser::expectKeyword(const QString& kw)
{
    const Token& t = peek();
    if (t.kind == TokenKind::Keyword && t.text == kw)
    {
        advance();
        return true;
    }
    fail(QStringLiteral("expected '%1' but found '%2'")
               .arg(kw, t.kind == TokenKind::End ? QString("end of script") : t.text)
               ,
         t.line);
    return false;
}

Token Parser::expectIdentifier()
{
    const Token& t = peek();
    if (t.kind == TokenKind::Identifier)
    {
        return advance();
    }
    if (t.kind == TokenKind::Keyword && isTypeKeyword(t.text))
    {
        // A type keyword used where an identifier is expected is a typo; the
        // caller's context decides whether it is a declaration prefix.
    }
    fail(QStringLiteral("expected an identifier but found '%1'")
               .arg(t.kind == TokenKind::End ? QString("end of script") : t.text)
               ,
         t.line);
    return Token{TokenKind::Error, QString(), t.line};
}

void Parser::fail(const QString& msg, int line)
{
    if (!m_error)
    {
        m_error = true;
        m_errorMsg = msg;
        m_errorLine = line;
    }
}

ParseResult Parser::parse()
{
    ParseResult result;
    if (m_error)
    {
        result.ok = false;
        result.error = m_errorMsg;
        result.errorLine = m_errorLine;
        return result;
    }

    // Optional top-level wrapper: `begin <name> ... end` / `endscript`.
    if (peek().kind == TokenKind::Keyword && peek().text == "begin")
    {
        advance();
        // Optional script name (identifier or a keyword like `script`/`quest`).
        if (peek().kind == TokenKind::Identifier ||
            peek().kind == TokenKind::Keyword)
        {
            advance();
        }
    }

    result.statements = parseBlock({QStringLiteral("end"), QStringLiteral("endscript")});

    if (m_error)
    {
        result.ok = false;
        result.error = m_errorMsg;
        result.errorLine = m_errorLine;
        return result;
    }

    result.ok = true;
    return result;
}

std::vector<StmtPtr> Parser::parseBlock(const QStringList& terminators)
{
    std::vector<StmtPtr> stmts;
    while (!m_error)
    {
        const Token& t = peek();
        if (t.kind == TokenKind::End)
        {
            break;
        }
        if (t.kind == TokenKind::NewLine)
        {
            advance();
            continue;
        }
        if (isTerminator(terminators))
        {
            break;
        }
        StmtPtr s = parseStatement();
        if (m_error)
        {
            return stmts;
        }
        stmts.push_back(std::move(s));
    }
    return stmts;
}

StmtPtr Parser::parseStatement()
{
    const Token& t = peek();
    if (t.kind == TokenKind::Keyword)
    {
        if (t.text == "function")
        {
            return parseFunction();
        }
        if (t.text == "if")
        {
            return parseIf();
        }
        if (t.text == "while")
        {
            return parseWhile();
        }
        if (t.text == "for")
        {
            return parseFor();
        }
        if (t.text == "return")
        {
            return parseReturn();
        }
        if (t.text == "let")
        {
            return parseLet(true);
        }
        if (t.text == "set")
        {
            return parseLet(false);
        }
        // Unsupported declaration / control keywords in this subset.
        fail(QStringLiteral("unsupported statement '%1'")
                   .arg(t.text)
                   ,
             t.line);
        return std::make_unique<Statement>();
    }

    // Expression-based statement: either `a.b[i] = expr` (assignment) or a
    // bare expression statement.
    ExprPtr expr = parseExpression();
    if (m_error)
    {
        return std::make_unique<Statement>();
    }

    if (peek().kind == TokenKind::Operator && peek().text == "=")
    {
        advance();
        Statement* s = new Statement();
        s->kind = StmtKind::Let;
        s->lhs = std::move(expr);
        s->value = parseExpression();
        s->line = t.line;
        return std::unique_ptr<Statement>(s);
    }

    Statement* s = new Statement();
    s->kind = StmtKind::ExprStmt;
    s->value = std::move(expr);
    s->line = t.line;
    return std::unique_ptr<Statement>(s);
}

StmtPtr Parser::parseIf()
{
    advance(); // if
    Statement* s = new Statement();
    s->kind = StmtKind::If;
    s->line = peek().line;

    if (!expectOperator("("))
    {
        return std::unique_ptr<Statement>(s);
    }
    s->condition = parseExpression();
    if (!expectOperator(")"))
    {
        return std::unique_ptr<Statement>(s);
    }
    s->body = parseBlock({QStringLiteral("elseif"), QStringLiteral("else"),
                          QStringLiteral("endif")});
    if (m_error)
    {
        return std::unique_ptr<Statement>(s);
    }

    while (true)
    {
        const Token& t = peek();
        if (t.kind == TokenKind::Keyword && t.text == "elseif")
        {
            advance();
            if (!expectOperator("("))
            {
                return std::unique_ptr<Statement>(s);
            }
            ExprPtr c = parseExpression();
            if (!expectOperator(")"))
            {
                return std::unique_ptr<Statement>(s);
            }
            std::vector<StmtPtr> b = parseBlock(
                {QStringLiteral("elseif"), QStringLiteral("else"),
                 QStringLiteral("endif")});
            if (m_error)
            {
                return std::unique_ptr<Statement>(s);
            }
            s->branches.emplace_back(std::move(c), std::move(b));
        }
        else if (t.kind == TokenKind::Keyword && t.text == "else")
        {
            advance();
            s->elseBody = parseBlock({QStringLiteral("endif")});
            if (m_error)
            {
                return std::unique_ptr<Statement>(s);
            }
            break;
        }
        else if (t.kind == TokenKind::Keyword && t.text == "endif")
        {
            break;
        }
        else
        {
            fail(QStringLiteral("expected elseif/else/endif but found '%1'")
                       .arg(t.kind == TokenKind::End ? QString("end of script")
                                                     : t.text)
                       ,
                 t.line);
            return std::unique_ptr<Statement>(s);
        }
    }
    // Consume `endif` if we stopped on it (the else-branch block consumed it
    // only when there was an else; without an else we break on it here).
    if (peek().kind == TokenKind::Keyword && peek().text == "endif")
    {
        advance();
    }
    return std::unique_ptr<Statement>(s);
}

StmtPtr Parser::parseWhile()
{
    advance(); // while
    Statement* s = new Statement();
    s->kind = StmtKind::While;
    s->line = peek().line;

    if (!expectOperator("("))
    {
        return std::unique_ptr<Statement>(s);
    }
    s->condition = parseExpression();
    if (!expectOperator(")"))
    {
        return std::unique_ptr<Statement>(s);
    }
    s->body = parseBlock({QStringLiteral("endwhile")});
    if (peek().kind == TokenKind::Keyword && peek().text == "endwhile")
    {
        advance();
    }
    return std::unique_ptr<Statement>(s);
}

StmtPtr Parser::parseFor()
{
    advance(); // for
    Statement* s = new Statement();
    s->kind = StmtKind::For;
    s->line = peek().line;

    // Optional explicit type, e.g. `for int i = ...`.
    if (peek().kind == TokenKind::Keyword && isTypeKeyword(peek().text))
    {
        advance();
    }
    Token var = expectIdentifier();
    if (m_error)
    {
        return std::unique_ptr<Statement>(s);
    }
    s->forVar = var.text;

    if (!expectOperator("="))
    {
        return std::unique_ptr<Statement>(s);
    }
    s->forInit = parseExpression();
    if (m_error)
    {
        return std::unique_ptr<Statement>(s);
    }

    const Token& toTok = peek();
    if (toTok.kind == TokenKind::Identifier && toTok.text.toLower() == "to")
    {
        advance();
    }
    else
    {
        fail(QStringLiteral("expected 'to' in for-loop but found '%1'")
                   .arg(toTok.kind == TokenKind::End ? QString("end of script")
                                                      : toTok.text)
                   ,
             toTok.line);
        return std::unique_ptr<Statement>(s);
    }

    s->forEnd = parseExpression();
    if (m_error)
    {
        return std::unique_ptr<Statement>(s);
    }
    s->body = parseBlock({QStringLiteral("endfor")});
    if (peek().kind == TokenKind::Keyword && peek().text == "endfor")
    {
        advance();
    }
    return std::unique_ptr<Statement>(s);
}

StmtPtr Parser::parseReturn()
{
    advance(); // return
    Statement* s = new Statement();
    s->kind = StmtKind::Return;
    s->line = peek().line;

    const Token& t = peek();
    const bool noValue =
        t.kind == TokenKind::End || t.kind == TokenKind::NewLine ||
        (t.kind == TokenKind::Keyword &&
         (t.text == "end" || t.text == "endif" || t.text == "endwhile" ||
          t.text == "endfor" || t.text == "endfunction" ||
          t.text == "endscript" || t.text == "else" || t.text == "elseif"));
    if (!noValue)
    {
        s->value = parseExpression();
    }
    return std::unique_ptr<Statement>(s);
}

StmtPtr Parser::parseLet(bool declares)
{
    advance(); // let / set
    Statement* s = new Statement();
    s->kind = StmtKind::Let;
    s->declares = declares;
    s->line = peek().line;

    // Optional type, e.g. `let float x = 1.0`.
    if (peek().kind == TokenKind::Keyword && isTypeKeyword(peek().text))
    {
        advance();
    }
    Token var = expectIdentifier();
    if (m_error)
    {
        return std::unique_ptr<Statement>(s);
    }
    Expr* id = new Expr();
    id->kind = ExprKind::Identifier;
    id->name = var.text;
    id->line = var.line;
    s->lhs.reset(id);

    if (!expectOperator("="))
    {
        return std::unique_ptr<Statement>(s);
    }
    s->value = parseExpression();
    return std::unique_ptr<Statement>(s);
}

StmtPtr Parser::parseFunction()
{
    advance(); // function
    Statement* s = new Statement();
    s->kind = StmtKind::Function;
    s->line = peek().line;

    Token name = expectIdentifier();
    if (m_error)
    {
        return std::unique_ptr<Statement>(s);
    }
    s->funcName = name.text;

    if (!expectOperator("("))
    {
        return std::unique_ptr<Statement>(s);
    }
    if (!(peek().kind == TokenKind::Operator && peek().text == ")"))
    {
        while (true)
        {
            // Optional parameter type.
            if (peek().kind == TokenKind::Keyword && isTypeKeyword(peek().text))
            {
                advance();
            }
            Token p = expectIdentifier();
            if (m_error)
            {
                return std::unique_ptr<Statement>(s);
            }
            s->params.append(p.text);
            if (peek().kind == TokenKind::Operator && peek().text == ",")
            {
                advance();
                continue;
            }
            break;
        }
    }
    if (!expectOperator(")"))
    {
        return std::unique_ptr<Statement>(s);
    }
    s->body = parseBlock({QStringLiteral("endfunction")});
    if (peek().kind == TokenKind::Keyword && peek().text == "endfunction")
    {
        advance();
    }
    return std::unique_ptr<Statement>(s);
}

ExprPtr Parser::parseExpression()
{
    return parseOr();
}

ExprPtr Parser::parseOr()
{
    ExprPtr left = parseAnd();
    if (m_error)
    {
        return left;
    }
    while (peek().kind == TokenKind::Operator && peek().text == "||")
    {
        advance();
        ExprPtr right = parseAnd();
        if (m_error)
        {
            return right;
        }
        Expr* e = new Expr();
        e->kind = ExprKind::BinaryOp;
        e->op = "||";
        e->left = std::move(left);
        e->right = std::move(right);
        e->line = e->left->line;
        left.reset(e);
    }
    return left;
}

ExprPtr Parser::parseAnd()
{
    ExprPtr left = parseEquality();
    if (m_error)
    {
        return left;
    }
    while (peek().kind == TokenKind::Operator && peek().text == "&&")
    {
        advance();
        ExprPtr right = parseEquality();
        if (m_error)
        {
            return right;
        }
        Expr* e = new Expr();
        e->kind = ExprKind::BinaryOp;
        e->op = "&&";
        e->left = std::move(left);
        e->right = std::move(right);
        e->line = e->left->line;
        left.reset(e);
    }
    return left;
}

ExprPtr Parser::parseEquality()
{
    ExprPtr left = parseRelational();
    if (m_error)
    {
        return left;
    }
    while (peek().kind == TokenKind::Operator &&
           (peek().text == "==" || peek().text == "!="))
    {
        QString op = peek().text;
        advance();
        ExprPtr right = parseRelational();
        if (m_error)
        {
            return right;
        }
        Expr* e = new Expr();
        e->kind = ExprKind::BinaryOp;
        e->op = op;
        e->left = std::move(left);
        e->right = std::move(right);
        e->line = e->left->line;
        left.reset(e);
    }
    return left;
}

ExprPtr Parser::parseRelational()
{
    ExprPtr left = parseAdditive();
    if (m_error)
    {
        return left;
    }
    while (peek().kind == TokenKind::Operator &&
           (peek().text == "<" || peek().text == ">" || peek().text == "<=" ||
            peek().text == ">="))
    {
        QString op = peek().text;
        advance();
        ExprPtr right = parseAdditive();
        if (m_error)
        {
            return right;
        }
        Expr* e = new Expr();
        e->kind = ExprKind::BinaryOp;
        e->op = op;
        e->left = std::move(left);
        e->right = std::move(right);
        e->line = e->left->line;
        left.reset(e);
    }
    return left;
}

ExprPtr Parser::parseAdditive()
{
    ExprPtr left = parseMultiplicative();
    if (m_error)
    {
        return left;
    }
    while (peek().kind == TokenKind::Operator &&
           (peek().text == "+" || peek().text == "-"))
    {
        QString op = peek().text;
        advance();
        ExprPtr right = parseMultiplicative();
        if (m_error)
        {
            return right;
        }
        Expr* e = new Expr();
        e->kind = ExprKind::BinaryOp;
        e->op = op;
        e->left = std::move(left);
        e->right = std::move(right);
        e->line = e->left->line;
        left.reset(e);
    }
    return left;
}

ExprPtr Parser::parseMultiplicative()
{
    ExprPtr left = parseUnary();
    if (m_error)
    {
        return left;
    }
    while (peek().kind == TokenKind::Operator &&
           (peek().text == "*" || peek().text == "/" || peek().text == "%"))
    {
        QString op = peek().text;
        advance();
        ExprPtr right = parseUnary();
        if (m_error)
        {
            return right;
        }
        Expr* e = new Expr();
        e->kind = ExprKind::BinaryOp;
        e->op = op;
        e->left = std::move(left);
        e->right = std::move(right);
        e->line = e->left->line;
        left.reset(e);
    }
    return left;
}

ExprPtr Parser::parseUnary()
{
    const Token& t = peek();
    if (t.kind == TokenKind::Operator &&
        (t.text == "-" || t.text == "!" || t.text == "~"))
    {
        advance();
        ExprPtr operand = parseUnary();
        if (m_error)
        {
            return operand;
        }
        Expr* e = new Expr();
        e->kind = ExprKind::UnaryOp;
        e->op = t.text;
        e->operand = std::move(operand);
        e->line = t.line;
        return std::unique_ptr<Expr>(e);
    }
    return parsePostfix();
}

ExprPtr Parser::parsePostfix()
{
    ExprPtr base = parsePrimary();
    if (m_error)
    {
        return base;
    }
    while (true)
    {
        const Token& t = peek();
        if (t.kind == TokenKind::Operator && t.text == "(")
        {
            advance();
            Expr* e = new Expr();
            e->kind = ExprKind::Call;
            e->callee = std::move(base);
            e->line = t.line;
            if (!(peek().kind == TokenKind::Operator && peek().text == ")"))
            {
                while (true)
                {
                    ExprPtr arg = parseExpression();
                    if (m_error)
                    {
                        return std::unique_ptr<Expr>(e);
                    }
                    e->args.push_back(std::move(arg));
                    if (peek().kind == TokenKind::Operator && peek().text == ",")
                    {
                        advance();
                        continue;
                    }
                    break;
                }
            }
            if (!expectOperator(")"))
            {
                return std::unique_ptr<Expr>(e);
            }
            base.reset(e);
        }
        else if (t.kind == TokenKind::Operator && t.text == ".")
        {
            advance();
            Token field = expectIdentifier();
            if (m_error)
            {
                return base;
            }
            Expr* e = new Expr();
            e->kind = ExprKind::FieldAccess;
            e->base = std::move(base);
            e->name = field.text;
            e->line = t.line;
            base.reset(e);
        }
        else if (t.kind == TokenKind::Operator && t.text == "[")
        {
            advance();
            Expr* e = new Expr();
            e->kind = ExprKind::Index;
            e->base = std::move(base);
            e->line = t.line;
            e->index = parseExpression();
            if (!expectOperator("]"))
            {
                return std::unique_ptr<Expr>(e);
            }
            base.reset(e);
        }
        else
        {
            break;
        }
    }
    return base;
}

ExprPtr Parser::parsePrimary()
{
    const Token& t = peek();
    switch (t.kind)
    {
    case TokenKind::Integer:
    {
        advance();
        Expr* e = new Expr();
        e->kind = ExprKind::LiteralInt;
        e->intValue = t.text.toLongLong();
        e->line = t.line;
        return std::unique_ptr<Expr>(e);
    }
    case TokenKind::Floating:
    {
        advance();
        Expr* e = new Expr();
        e->kind = ExprKind::LiteralFloat;
        e->floatValue = t.text.toDouble();
        e->line = t.line;
        return std::unique_ptr<Expr>(e);
    }
    case TokenKind::String:
    {
        advance();
        Expr* e = new Expr();
        e->kind = ExprKind::LiteralString;
        e->stringValue = t.text;
        e->line = t.line;
        return std::unique_ptr<Expr>(e);
    }
    case TokenKind::Identifier:
    {
        advance();
        Expr* e = new Expr();
        e->kind = ExprKind::Identifier;
        e->name = t.text;
        e->line = t.line;
        return std::unique_ptr<Expr>(e);
    }
    case TokenKind::Keyword:
    {
        if (t.text == "true" || t.text == "false")
        {
            advance();
            Expr* e = new Expr();
            e->kind = ExprKind::LiteralBool;
            e->boolValue = (t.text == "true");
            e->line = t.line;
            return std::unique_ptr<Expr>(e);
        }
        if (t.text == "nil" || t.text == "null")
        {
            advance();
            Expr* e = new Expr();
            e->kind = ExprKind::LiteralNil;
            e->line = t.line;
            return std::unique_ptr<Expr>(e);
        }
        // `if`/`while`/etc. appearing in expression position is an error.
        fail(QStringLiteral("unexpected keyword '%1' in expression")
                   .arg(t.text)
                   ,
             t.line);
        return std::make_unique<Expr>();
    }
    case TokenKind::Operator:
    {
        if (t.text == "(")
        {
            advance();
            ExprPtr inner = parseExpression();
            if (!expectOperator(")"))
            {
                return std::make_unique<Expr>();
            }
            // Parentheses are grouping only; return the inner expression.
            return inner;
        }
        fail(QStringLiteral("unexpected operator '%1' in expression")
                   .arg(t.text)
                   ,
             t.line);
        return std::make_unique<Expr>();
    }
    default:
        fail(QStringLiteral("unexpected '%1' in expression")
                   .arg(t.kind == TokenKind::End ? QString("end of script") : t.text)
                   ,
             t.line);
        return std::make_unique<Expr>();
    }
}

ParseResult parse(const QString& source)
{
    return Parser(source).parse();
}

} // namespace ObScript
