#include "inifile.hpp"

#include <QFile>
#include <QTextStream>

namespace {
// Lookup keys are normalised, never returned, so returning by value costs
// nothing measurable and avoids handing out a reference to a temporary.
QString normKey(const QString& s)
{
    return s.toLower();
}
} // namespace

bool IniFile::load(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    m_sections.clear();

    QTextStream in(&file);
    QString currentSection = normKey(QStringLiteral(""));
    // Anything before the first [Section] header lands here, rather than being
    // discarded: a malformed file should still yield whatever it did declare.
    auto& root = m_sections[currentSection];

    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;
        const QChar first = line.at(0);
        if (first == QLatin1Char(';') || first == QLatin1Char('#'))
            continue;

        if (first == QLatin1Char('['))
        {
            const int close = line.lastIndexOf(QLatin1Char(']'));
            if (close <= 1)
                continue; // no closing bracket: not a header, so not a header
            currentSection = normKey(line.mid(1, close - 1).trimmed());
            root = m_sections[currentSection];
            continue;
        }

        const int eq = line.indexOf(QLatin1Char('='));
        if (eq <= 0)
            continue; // no key, or an empty key
        const QString key = normKey(line.left(eq).trimmed());
        if (key.isEmpty())
            continue;
        QString value = line.mid(eq + 1).trimmed();
        // Quoted values keep their spaces; the quoting itself is not data.
        if (value.size() >= 2 && value.startsWith(QLatin1Char('"'))
            && value.endsWith(QLatin1Char('"'))) {
            value = value.mid(1, value.size() - 2);
        }
        m_sections[currentSection][key].append(value);
    }
    return true;
}

QStringList IniFile::sections() const
{
    return m_sections.keys();
}

bool IniFile::hasSection(const QString& section) const
{
    return m_sections.contains(normKey(section));
}

bool IniFile::hasKey(const QString& section, const QString& key) const
{
    const auto it = m_sections.constFind(normKey(section));
    return it != m_sections.constEnd() && it.value().contains(normKey(key));
}

QString IniFile::value(const QString& section, const QString& key,
                       const QString& defaultValue) const
{
    const QStringList all = values(section, key);
    return all.isEmpty() ? defaultValue : all.first();
}

QStringList IniFile::values(const QString& section, const QString& key) const
{
    const auto it = m_sections.constFind(normKey(section));
    if (it == m_sections.constEnd())
        return {};
    return it.value().value(normKey(key));
}

QStringList IniFile::valueList(const QString& section, const QString& key) const
{
    QStringList out;
    // Every occurrence of the key contributes, not just the first: a repeated key
    // carrying a list should not lose its later lines.
    for (const QString& raw : values(section, key))
    {
        for (const QString& part : raw.split(QLatin1Char(',')))
        {
            const QString trimmed = part.trimmed();
            if (!trimmed.isEmpty())
                out.append(trimmed);
        }
    }
    return out;
}

int IniFile::intValue(const QString& section, const QString& key, int defaultValue) const
{
    bool ok = false;
    const int parsed = value(section, key).trimmed().toInt(&ok);
    return ok ? parsed : defaultValue;
}

bool IniFile::boolValue(const QString& section, const QString& key, bool defaultValue) const
{
    if (!hasKey(section, key))
        return defaultValue;
    const QString v = value(section, key).trimmed().toLower();
    if (v == QStringLiteral("1") || v == QStringLiteral("true")
        || v == QStringLiteral("yes") || v == QStringLiteral("on")) {
        return true;
    }
    if (v == QStringLiteral("0") || v == QStringLiteral("false")
        || v == QStringLiteral("no") || v == QStringLiteral("off")) {
        return false;
    }
    return defaultValue;
}