#include "spellchecker.hpp"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

int SpellChecker::editDistance(const QString& a, const QString& b)
{
    const int m = a.size();
    const int n = b.size();
    if (m == 0) return n;
    if (n == 0) return m;

    QVector<int> prev(n + 1), curr(n + 1);
    for (int j = 0; j <= n; ++j)
        prev[j] = j;

    for (int i = 1; i <= m; ++i)
    {
        curr[0] = i;
        for (int j = 1; j <= n; ++j)
        {
            const int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            curr[j] = qMin(qMin(curr[j - 1] + 1, prev[j] + 1), prev[j - 1] + cost);
        }
        prev = curr;
    }
    return prev[n];
}

int SpellChecker::loadDictionary(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return 0;

    int added = 0;
    QTextStream in(&file);
    while (!in.atEnd())
    {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(';') || line.startsWith('#'))
            continue;
        addWord(line);
        ++added;
    }
    return added;
}

void SpellChecker::addWord(const QString& word)
{
    const QString w = word.trimmed();
    if (w.isEmpty())
        return;
    m_words.insert(w.toLower());
}

bool SpellChecker::isKnown(const QString& word) const
{
    return m_words.contains(word.trimmed().toLower());
}

QStringList SpellChecker::unknownWords(const QString& text) const
{
    QStringList result;
    static const QRegularExpression wordRe(
        QStringLiteral("[A-Za-z]+(?:'[A-Za-z]+)*"));

    QRegularExpressionMatchIterator it = wordRe.globalMatch(text);
    while (it.hasNext())
    {
        const QString word = it.next().captured();
        if (!isKnown(word))
            result.append(word);
    }
    return result;
}

QStringList SpellChecker::suggestions(const QString& word, int maxDistance,
                                      int maxSuggestions) const
{
    const QString lower = word.trimmed().toLower();
    if (lower.isEmpty())
        return QStringList();

    // Candidates closer first; ties broken alphabetically.
    QVector<QPair<int, QString>> scored;
    for (const QString& candidate : m_words)
    {
        if (candidate == lower)
            continue;
        const int distance = editDistance(lower, candidate);
        if (distance <= maxDistance)
            scored.append(qMakePair(distance, candidate));
    }
    std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) {
        if (a.first != b.first) return a.first < b.first;
        return a.second < b.second;
    });

    QStringList out;
    for (const auto& pair : scored)
    {
        out.append(pair.second);
        if (out.size() >= maxSuggestions)
            break;
    }
    return out;
}
