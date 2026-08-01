#ifndef SPELLCHECKER_H
#define SPELLCHECKER_H

#include <QString>
#include <QStringList>
#include <QSet>
#include <QHash>

// SpellChecker provides dictionary-based spell checking for the script and
// dialogue editors, mirroring the real Creation Kit's built-in Sentry SSCE
// dictionaries. It loads word lists (one word per line, optional language
// tags) and reports unknown words with edit-distance suggestions.
class SpellChecker
{
public:
    // Loads words from a text file (one per line). Lines starting with ';'
    // or '#' are ignored. Returns the number of words added.
    int loadDictionary(const QString& path);

    // Adds a single word (trimmed; empty words ignored).
    void addWord(const QString& word);

    // True if the word is in the dictionary (case-insensitive).
    bool isKnown(const QString& word) const;

    // Returns the words in 'text' that are not in the dictionary, in order
    // of appearance. Numbers and words with digits are ignored.
    QStringList unknownWords(const QString& text) const;

    // Returns up to 'maxSuggestions' dictionary words within
    // 'maxDistance' edits of the given word, ordered by closeness.
    QStringList suggestions(const QString& word, int maxDistance = 2,
                            int maxSuggestions = 5) const;

    // Number of words loaded.
    int wordCount() const { return m_words.size(); }

    // Clears all words.
    void clear() { m_words.clear(); }

private:
    static int editDistance(const QString& a, const QString& b);

    QSet<QString> m_words;
};

#endif // SPELLCHECKER_H
