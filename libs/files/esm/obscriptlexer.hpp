#pragma once

#include <QSet>
#include <QString>
#include <QVector>

// Lexical analyzer for a practical subset of Bethesda's OBScript
// (the .scm / inline script language used by ESM/ESP records). It tokenizes
// reserved words, identifiers, integer/float literals, string literals,
// operators, and newlines (which are statement separators). Comments start
// with ';' and run to the end of the line. The lexer is deliberately
// source-only: it does not validate syntax, it just splits text into a
// deterministic token stream a parser can consume.
namespace ObScript
{

const QSet<QString>& keywords();

enum class TokenKind
{
    End,
    Keyword,
    Identifier,
    Integer,
    Floating,
    String,
    Operator,
    NewLine,
    Error
};

struct Token
{
    TokenKind kind = TokenKind::End;
    QString text; // raw source text; keywords lower-cased; strings unescaped
    int line = 1;
};

class Lexer
{
public:
    explicit Lexer(const QString& source);

    Token next();
    QVector<Token> tokenizeAll();

    bool hasError() const { return m_error; }
    QString errorMessage() const { return m_errorMsg; }

private:
    char peek() const;
    char peek(int off) const;
    char advance();
    bool atEnd() const;

    Token readNumber();
    Token readString();
    Token readIdentifierOrKeyword();

    QString m_src;
    int m_pos = 0;
    int m_line = 1;
    bool m_error = false;
    QString m_errorMsg;
};

} // namespace ObScript
