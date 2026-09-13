#include "obscripthighlighter.hpp"

#include <QColor>
#include <QSet>
#include <QTextDocument>

namespace
{

const QSet<QString>& controlFlowWords()
{
    static const QSet<QString> k = {
        "if", "elseif", "else", "endif", "while", "endwhile",
        "for", "endfor", "return", "function", "endfunction",
        "begin", "end", "endscript", "case", "endcase", "switch",
        "endswitch", "try", "catch", "endtry"
    };
    return k;
}

const QSet<QString>& typeWords()
{
    static const QSet<QString> k = {
        "float", "int", "bool", "string", "objectreference", "keyword",
        "actorvalue", "form", "formlist", "package", "quest", "sound",
        "spell", "magiceffect", "script", "ref", "reference", "local",
        "global", "globalint", "globalfloat", "globalstring",
        "globalactorvalue", "globalform", "globalkeyword", "globalpackage",
        "globalscript", "globalspell", "globalmagiceffect", "globalformlist",
        "globalquest", "globalsound", "property", "event"
    };
    return k;
}

const QSet<QString>& otherWords()
{
    static const QSet<QString> k = {
        "const", "static", "pure", "hidden", "implicit", "auto",
        "let", "set", "true", "false", "nil", "null"
    };
    return k;
}

bool isIdentStart(QChar c)
{
    return c.isLetter() || c == '_';
}

bool isIdentChar(QChar c)
{
    return c.isLetterOrNumber() || c == '_';
}

bool isHex(QChar c)
{
    return c.isDigit() || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool isOperator(QChar c)
{
    static const QString ops = QStringLiteral("+-*/%=<>!&|~^");
    return ops.contains(c);
}

} // namespace

QVector<ObScriptHighlightSpan> classifyObScriptLine(const QString& line)
{
    QVector<ObScriptHighlightSpan> spans;
    const int n = line.size();
    int i = 0;

    while (i < n)
    {
        const QChar c = line.at(i);

        if (c == ';')
        {
            spans.append({i, n - i, ObScriptHighlightKind::Comment});
            break;
        }

        if (c == '"')
        {
            int j = i + 1;
            while (j < n)
            {
                if (line.at(j) == '\\')
                {
                    j += 2;
                    continue;
                }
                if (line.at(j) == '"')
                {
                    ++j;
                    break;
                }
                ++j;
            }
            spans.append({i, j - i, ObScriptHighlightKind::String});
            i = j;
            continue;
        }

        if (isIdentStart(c))
        {
            int j = i;
            while (j < n && isIdentChar(line.at(j)))
            {
                ++j;
            }
            const QString lower = line.mid(i, j - i).toLower();
            bool matched = true;
            ObScriptHighlightKind kind = ObScriptHighlightKind::Keyword;
            if (controlFlowWords().contains(lower))
            {
                kind = ObScriptHighlightKind::ControlFlow;
            }
            else if (typeWords().contains(lower))
            {
                kind = ObScriptHighlightKind::Type;
            }
            else if (otherWords().contains(lower))
            {
                kind = ObScriptHighlightKind::Keyword;
            }
            else
            {
                matched = false;
            }
            if (matched)
            {
                spans.append({i, j - i, kind});
            }
            i = j;
            continue;
        }

        if (c.isDigit() || (c == '.' && i + 1 < n && line.at(i + 1).isDigit()))
        {
            int j = i;
            if (c == '0' && j + 1 < n && (line.at(j + 1) == 'x' || line.at(j + 1) == 'X'))
            {
                j += 2;
                while (j < n && isHex(line.at(j)))
                {
                    ++j;
                }
            }
            else
            {
                while (j < n && (line.at(j).isDigit() || line.at(j) == '.'))
                {
                    ++j;
                }
                if (j < n && (line.at(j) == 'e' || line.at(j) == 'E'))
                {
                    ++j;
                    if (j < n && (line.at(j) == '+' || line.at(j) == '-'))
                    {
                        ++j;
                    }
                    while (j < n && line.at(j).isDigit())
                    {
                        ++j;
                    }
                }
                if (j < n && (line.at(j) == 'f' || line.at(j) == 'F'))
                {
                    ++j;
                }
            }
            spans.append({i, j - i, ObScriptHighlightKind::Number});
            i = j;
            continue;
        }

        if (isOperator(c))
        {
            int j = i;
            while (j < n && isOperator(line.at(j)))
            {
                ++j;
            }
            spans.append({i, j - i, ObScriptHighlightKind::Operator});
            i = j;
            continue;
        }

        ++i;
    }

    return spans;
}

ObScriptHighlighter::ObScriptHighlighter(QTextDocument* document)
    : QSyntaxHighlighter(document)
{
    m_formats[int(ObScriptHighlightKind::ControlFlow)].setForeground(QColor(0x7a, 0x1f, 0x9e));
    m_formats[int(ObScriptHighlightKind::ControlFlow)].setFontWeight(QFont::Bold);

    m_formats[int(ObScriptHighlightKind::Type)].setForeground(QColor(0x00, 0x6b, 0x6b));

    m_formats[int(ObScriptHighlightKind::Keyword)].setForeground(QColor(0x15, 0x65, 0xc0));
    m_formats[int(ObScriptHighlightKind::Keyword)].setFontWeight(QFont::Bold);

    m_formats[int(ObScriptHighlightKind::String)].setForeground(QColor(0xa3, 0x15, 0x15));
    m_formats[int(ObScriptHighlightKind::Comment)].setForeground(QColor(0x60, 0x80, 0x60));
    m_formats[int(ObScriptHighlightKind::Comment)].setFontItalic(true);

    m_formats[int(ObScriptHighlightKind::Number)].setForeground(QColor(0x0b, 0x60, 0x8a));
    m_formats[int(ObScriptHighlightKind::Operator)].setForeground(QColor(0x55, 0x55, 0x55));
}

void ObScriptHighlighter::highlightBlock(const QString& text)
{
    const QVector<ObScriptHighlightSpan> spans = classifyObScriptLine(text);
    for (const ObScriptHighlightSpan& span : spans)
    {
        setFormat(span.start, span.length, m_formats[int(span.kind)]);
    }
}
