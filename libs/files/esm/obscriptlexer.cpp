#include "obscriptlexer.hpp"

#include <QSet>
#include <QString>

namespace ObScript
{

const QSet<QString>& keywords()
{
    static const QSet<QString> k = {
        // types / declaration
        "float", "int", "bool", "string", "objectreference", "keyword",
        "actorvalue", "form", "formlist", "package", "quest", "sound",
        "spell", "magiceffect", "script", "ref", "reference", "local",
        "global", "globalint", "globalfloat", "globalstring",
        "globalactorvalue", "globalform", "globalkeyword", "globalpackage",
        "globalscript", "globalspell", "globalmagiceffect", "globalformlist",
        "globalquest", "globalsound", "property", "event",
        // control flow
        "begin", "end", "function", "endfunction", "endscript", "return",
        "if", "else", "elseif", "endif", "while", "endwhile", "for",
        "endfor", "case", "endcase", "switch", "endswitch", "try", "catch",
        "endtry",
        // modifiers / literals
        "const", "static", "pure", "hidden", "implicit", "auto",
        "let", "set",
        "true", "false", "nil", "null"
    };
    return k;
}

namespace
{

bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

bool isHexDigit(char c)
{
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool isIdentStart(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool isIdentChar(char c)
{
    return isIdentStart(c) || isDigit(c);
}

bool isOperatorChar(char c)
{
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
           c == '<' || c == '>' || c == '=' || c == '!' || c == '&' ||
           c == '|' || c == '(' || c == ')' || c == '[' || c == ']' ||
           c == '{' || c == '}' || c == '.' || c == ',' || c == ':' ||
           c == '?';
}

} // namespace

Lexer::Lexer(const QString& source) : m_src(source)
{
}

char Lexer::peek() const
{
    return m_pos < m_src.size() ? m_src[m_pos].toLatin1() : 0;
}

char Lexer::peek(int off) const
{
    return m_pos + off < m_src.size() ? m_src[m_pos + off].toLatin1() : 0;
}

char Lexer::advance()
{
    char c = peek();
    if (c == '\n')
    {
        ++m_line;
    }
    if (m_pos < m_src.size())
    {
        ++m_pos;
    }
    return c;
}

bool Lexer::atEnd() const
{
    return m_pos >= m_src.size();
}

Token Lexer::next()
{
    while (!atEnd() && (peek() == ' ' || peek() == '\t' || peek() == '\r'))
    {
        advance();
    }

    if (atEnd())
    {
        return Token{TokenKind::End, QString(), m_line};
    }

    char c = peek();

    if (c == '\n')
    {
        const int line = m_line;
        while (!atEnd() &&
               (peek() == '\n' || peek() == ' ' || peek() == '\t' ||
                peek() == '\r'))
        {
            if (peek() == '\n')
            {
                ++m_line;
            }
            advance();
        }
        return Token{TokenKind::NewLine, QStringLiteral("\n"), line};
    }

    if (c == ';')
    {
        while (!atEnd() && peek() != '\n')
        {
            advance();
        }
        return next();
    }

    if (c == '"')
    {
        return readString();
    }

    if (isDigit(c) || (c == '.' && isDigit(peek(1))))
    {
        return readNumber();
    }

    if (isIdentStart(c))
    {
        return readIdentifierOrKeyword();
    }

    Token t;
    t.line = m_line;
    const QString two = m_src.mid(m_pos, 2);
    if (two == "==" || two == "!=" || two == "<=" || two == ">=" ||
        two == "&&" || two == "||" || two == "+=" || two == "-=" ||
        two == "*=" || two == "/=" || two == "%=" || two == "&=" ||
        two == "|=")
    {
        t.kind = TokenKind::Operator;
        t.text = two;
        advance();
        advance();
        return t;
    }

    if (isOperatorChar(c))
    {
        t.kind = TokenKind::Operator;
        t.text = QString(QChar(c));
        advance();
        return t;
    }

    t.kind = TokenKind::Error;
    t.text = QString(QChar(c));
    m_error = true;
    m_errorMsg = QStringLiteral("Unexpected character %1 at line %2")
                      .arg(QChar(c))
                      .arg(m_line);
    advance();
    return t;
}

Token Lexer::readNumber()
{
    const int line = m_line;
    const int start = m_pos;
    bool isHex = false;

    if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'X'))
    {
        isHex = true;
        advance();
        advance();
        while (isHexDigit(peek()))
        {
            advance();
        }
    }
    else
    {
        while (isDigit(peek()))
        {
            advance();
        }
        bool floating = false;
        if (peek() == '.')
        {
            floating = true;
            advance();
            while (isDigit(peek()))
            {
                advance();
            }
        }
        if (peek() == 'e' || peek() == 'E')
        {
            floating = true;
            advance();
            if (peek() == '+' || peek() == '-')
            {
                advance();
            }
            while (isDigit(peek()))
            {
                advance();
            }
        }
        if (peek() == 'f' || peek() == 'F')
        {
            floating = true;
            advance();
        }
        (void)floating;
    }

    const QString raw = m_src.mid(start, m_pos - start);
    Token t;
    t.line = line;
    t.text = raw;
    t.kind = (isHex || !(raw.contains('.') || raw.contains('e') ||
                         raw.contains('E') || raw.contains('f') ||
                         raw.contains('F')))
                 ? TokenKind::Integer
                 : TokenKind::Floating;
    return t;
}

Token Lexer::readString()
{
    const int line = m_line;
    advance(); // opening quote
    QString content;
    while (!atEnd())
    {
        char c = advance();
        if (c == '"')
        {
            break;
        }
        if (c == '\\' && !atEnd())
        {
            char e = advance();
            switch (e)
            {
            case 'n':
                content += '\n';
                break;
            case 't':
                content += '\t';
                break;
            case 'r':
                content += '\r';
                break;
            case '"':
                content += '"';
                break;
            case '\\':
                content += '\\';
                break;
            default:
                content += e;
                break;
            }
        }
        else
        {
            content += c;
        }
    }

    Token t;
    t.kind = TokenKind::String;
    t.text = content;
    t.line = line;
    return t;
}

Token Lexer::readIdentifierOrKeyword()
{
    const int line = m_line;
    const int start = m_pos;
    while (isIdentChar(peek()))
    {
        advance();
    }
    const QString raw = m_src.mid(start, m_pos - start);

    Token t;
    t.line = line;
    const QString lower = raw.toLower();
    if (keywords().contains(lower))
    {
        t.kind = TokenKind::Keyword;
        t.text = lower;
    }
    else
    {
        t.kind = TokenKind::Identifier;
        t.text = raw;
    }
    return t;
}

QVector<Token> Lexer::tokenizeAll()
{
    QVector<Token> out;
    while (true)
    {
        Token t = next();
        out.append(t);
        if (t.kind == TokenKind::End)
        {
            break;
        }
    }
    return out;
}

} // namespace ObScript
