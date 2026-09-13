#ifndef OBSCRIPTHIGHLIGHTER_HPP
#define OBSCRIPTHIGHLIGHTER_HPP

#include <QString>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>

class QTextDocument;

enum class ObScriptHighlightKind
{
    Keyword,
    ControlFlow,
    Type,
    String,
    Comment,
    Number,
    Operator
};

struct ObScriptHighlightSpan
{
    int start = 0;
    int length = 0;
    ObScriptHighlightKind kind = ObScriptHighlightKind::Keyword;
};

// Classifies one source line into colored spans. Pure logic (no QTextDocument),
// so it is directly unit-testable; ObScriptHighlighter applies the spans to a
// document. Unrecognized characters produce no span.
QVector<ObScriptHighlightSpan> classifyObScriptLine(const QString& line);

class ObScriptHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit ObScriptHighlighter(QTextDocument* document = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    QTextCharFormat m_formats[7];
};

#endif // OBSCRIPTHIGHLIGHTER_HPP
