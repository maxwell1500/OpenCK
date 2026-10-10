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

// Structural clone for assignment-target expressions. Assignment targets are
// identifiers, field accesses or index expressions; those are the shapes that
// can be re-evaluated on the right-hand side of a desugared `x += y`.
ExprPtr cloneExpr(const Expr* e)
{
    if (!e)
    {
        return nullptr;
    }
    Expr* c = new Expr();
    c->kind = e->kind;
    c->name = e->name;
    c->intValue = e->intValue;
    c->floatValue = e->floatValue;
    c->stringValue = e->stringValue;
    c->boolValue = e->boolValue;
    c->op = e->op;
    c->line = e->line;
    c->left = cloneExpr(e->left.get());
    c->right = cloneExpr(e->right.get());
    c->operand = cloneExpr(e->operand.get());
    for (const ExprPtr& a : e->args)
    {
        c->args.push_back(cloneExpr(a.get()));
    }
    c->base = cloneExpr(e->base.get());
    c->index = cloneExpr(e->index.get());
    return ExprPtr(c);
}

// True when the token at the cursor is a type prefix for a declaration:
// a built-in type keyword or a capitalised identifier (script type). A
// declaration `Foo x` and an expression statement are only distinguished by
// this: a bare identifier followed by another identifier cannot be an
// expression, so it must be a declaration.
bool isTypePrefixToken(const Token& t)
{
    if (t.kind == TokenKind::Keyword)
    {
        return isTypeKeyword(t.text);
    }
    if (t.kind == TokenKind::Identifier)
    {
        return !t.text.isEmpty() && t.text.at(0).isUpper();
    }
    return false;
}

// Keyword tokens are normalised to lower case by the lexer, which would turn
// `extends Quest` into `extends quest`. Script-type names are addressable, so
// restore the conventional leading capital for known script types.
QString capitalizeTypeKeyword(const Token& t)
{
    if (t.kind != TokenKind::Keyword || t.text.isEmpty())
    {
        return t.text;
    }
    return QString(t.text.at(0).toUpper()) + t.text.mid(1);
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

Token Parser::expectIdentifier(bool allowTypeKeyword)
{
    const Token& t = peek();
    if (t.kind == TokenKind::Identifier)
    {
        return advance();
    }
    if (allowTypeKeyword && t.kind == TokenKind::Keyword && isTypeKeyword(t.text))
    {
        // `extends Quest`, `Property ref ...`: script/extend names are
        // addressable identifiers even when they collude with the type
        // keyword set.
        return advance();
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

// True when the token after the next is the `Property` keyword, i.e. the
// current position begins a property declaration rather than an expression.
bool Parser::isPropertyKeywordAhead()
{
    const Token& n = peekAt(1);
    return n.kind == TokenKind::Keyword && n.text == QLatin1String("property");
}

// True when the cursor sits on `<type> <name>`, i.e. a declaration, rather
// than `<call>(` or `<name> = <value>`, which are expression statements. A
// bare capitalised identifier followed by an operator or a left paren is an
// expression, not a type prefix; only a following identifier makes it one.
bool Parser::atTypePrefix(const Token& t, QString* typeName) const
{
    if (!isTypePrefixToken(t))
    {
        return false;
    }
    // Built-in type keywords are unambiguous: `int x` can only be a
    // declaration, and no expression starts with a bare type keyword.
    if (t.kind == TokenKind::Keyword)
    {
        if (typeName)
        {
            *typeName = t.text;
        }
        return true;
    }
    // A script-typed declaration needs an identifier after the type name.
    const Token& next = peekAt(1);
    if (next.kind != TokenKind::Identifier)
    {
        return false;
    }
    if (typeName)
    {
        *typeName = t.text;
    }
    return true;
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
        if (t.text == "scriptname")
        {
            return parseHeader();
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
        // `<type> Property name [auto]`, e.g. `MyScript Property ref auto`
        // and `float Property float_param = 1.0`.
        if (isTypeKeyword(t.text) && isPropertyKeywordAhead())
        {
            const QString typeName = t.text;
            advance();
            return parseProperty(typeName, typeName, t.line);
        }
        // `<type> name` local/global declaration, e.g. `int i` / `MyScript q`.
        if (atTypePrefix(t, nullptr))
        {
            return parseLocalDeclaration(t.text, t.line);
        }
        // Unsupported declaration / control keywords in this subset.
        fail(QStringLiteral("unsupported statement '%1'")
                   .arg(t.text),
             t.line);
        return std::make_unique<Statement>();
    }

    // `SomeScript Property name auto` — script-typed property whose type is a
    // plain identifier (another script in the same plugin, or an engine
    // script type).
    if (t.kind == TokenKind::Identifier && isPropertyKeywordAhead())
    {
        const QString typeName = t.text;
        advance();
        return parseProperty(typeName, typeName, t.line);
    }

    // Expression-based statement: either `a.b[i] = expr` (assignment) or a
    // bare expression statement.
    ExprPtr expr = parseExpression();
    if (m_error)
    {
        return std::make_unique<Statement>();
    }

    // Plain or compound assignment (`=`, `+=`, `-=`, `*=`, `/=`). Compound
    // assignment desugars to `lhs = lhs <op> rhs` so downstream passes only
    // ever see plain Let nodes.
    static const QStringList kCompound = {QStringLiteral("+="), QStringLiteral("-="),
                                         QStringLiteral("*="), QStringLiteral("/="),
                                         QStringLiteral("%=")};
    if (peek().kind == TokenKind::Operator
        && (peek().text == "=" || kCompound.contains(peek().text)))
    {
        const QString op = advance().text;
        if (m_error)
        {
            return std::make_unique<Statement>();
        }

        Statement* s = new Statement();
        s->kind = StmtKind::Let;
        s->declares = false;
        s->line = t.line;
        s->lhs = std::move(expr);

        if (op == "=")
        {
            s->value = parseExpression();
            return std::unique_ptr<Statement>(s);
        }

        // `x += rhs` desugars to `x = x + rhs` so downstream passes only ever
        // see a plain Let with a BinaryOp value. The target expression is
        // duplicated structurally; only a handful of shapes occur as
        // assignment targets.
        Expr* combined = new Expr();
        combined->kind = ExprKind::BinaryOp;
        combined->op = op.left(op.size() - 1);
        combined->line = t.line;
        combined->left = cloneExpr(s->lhs.get());
        s->value.reset(combined);
        s->value->right = parseExpression();
        if (m_error)
        {
            return std::unique_ptr<Statement>(s);
        }
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

// `ScriptName <name> [extends <base>]` — the first line of every Papyrus
// source file. Recorded in the AST as a Header statement so semantic passes
// can validate the script's identity and its parent.
StmtPtr Parser::parseHeader()
{
    advance(); // scriptname
    Statement* s = new Statement();
    s->kind = StmtKind::Header;
    s->line = peek().line;

    Token name = expectIdentifier(true); // `ScriptName MyQuest`
    if (m_error)
    {
        return std::unique_ptr<Statement>(s);
    }
    s->funcName = name.text;

    if (peek().kind == TokenKind::Keyword && peek().text == "extends")
    {
        advance();
        Token base = expectIdentifier(true); // `extends Quest` / `extends Actor`
        if (m_error)
        {
            return std::unique_ptr<Statement>(s);
        }
        // The lexer normalises keyword tokens to lower case, which would
        // turn `extends Quest` into `extends quest`. Script types are
        // addressable names, so restore the display case.
        s->typeName = capitalizeTypeKeyword(base);
    }
    return std::unique_ptr<Statement>(s);
}

// `<type> Property <name> [auto] [= expr] [hidden|const]`.
StmtPtr Parser::parseProperty(const QString& typeName, const QString& typeText,
                              int line)
{
    Statement* s = new Statement();
    s->kind = StmtKind::Property;
    s->line = line;
    s->typeName = typeName;
    (void)typeText;

    if (!(peek().kind == TokenKind::Keyword && peek().text == "property"))
    {
        fail(QStringLiteral("expected 'Property' in property declaration"), line);
        return std::unique_ptr<Statement>(s);
    }
    advance(); // property

    Token name = expectIdentifier(true); // `Property ref ...` / `Property count ...`
    if (m_error)
    {
        return std::unique_ptr<Statement>(s);
    }
    s->funcName = name.text;

    // Modifiers in any order before the optional initializer.
    while (peek().kind == TokenKind::Keyword
           && (peek().text == "auto" || peek().text == "hidden"
               || peek().text == "const"))
    {
        if (peek().text == "auto")
        {
            s->isAuto = true;
        }
        advance();
    }

    if (peek().kind == TokenKind::Operator && peek().text == "=")
    {
        advance();
        s->value = parseExpression();
    }
    return std::unique_ptr<Statement>(s);
}

// `<type> <name> [= expr]` local/global variable declaration, e.g.
// `int i` or `MyScript questRef = someOther as MyScript`.
StmtPtr Parser::parseLocalDeclaration(const QString& typeText, int line)
{
    advance(); // type
    Statement* s = new Statement();
    s->kind = StmtKind::Local;
    s->line = line;
    s->typeName = typeText;

    Token name = expectIdentifier();
    if (m_error)
    {
        return std::unique_ptr<Statement>(s);
    }
    s->funcName = name.text;

    if (peek().kind == TokenKind::Operator && peek().text == "=")
    {
        advance();
        s->value = parseExpression();
    }
    return std::unique_ptr<Statement>(s);
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

QVector<PropertyDecl> propertyDecls(const ParseResult& program)
{
    QVector<PropertyDecl> out;
    for (const auto& stmt : program.statements)
    {
        if (!stmt || stmt->kind != StmtKind::Property)
        {
            continue;
        }
        PropertyDecl decl;
        decl.typeName = stmt->typeName;
        decl.name = stmt->funcName;
        decl.isAuto = stmt->isAuto;
        decl.line = stmt->line;
        out.append(decl);
    }
    return out;
}

} // namespace ObScript
