#ifndef RECORDFIELDPARSE_HPP
#define RECORDFIELDPARSE_HPP

#include <QString>

namespace openck {

/// Parses a FormID-style control value. Accepts "0x1A2B", "1A2B" and plain
/// decimal, because the Object Window shows these fields as bare hex and users
/// paste either form. Returns false and leaves `out` untouched when the text is
/// not a number, so a widget can refuse the edit instead of writing garbage.
inline bool parseFormId(const QString& text, quint32& out)
{
    QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return false;
    if (trimmed.startsWith(QLatin1String("0x"), Qt::CaseInsensitive))
        trimmed = trimmed.mid(2);
    if (trimmed.isEmpty())
        return false;
    bool ok = false;
    const uint value = trimmed.toUInt(&ok, 16);
    if (!ok)
        return false;
    out = quint32(value);
    return true;
}

/// Formats a FormID the way the Object Window displays it.
inline QString formatFormId(quint32 value)
{
    return QStringLiteral("0x%1").arg(value, 8, 16, QLatin1Char('0'));
}

} // namespace openck

#endif // RECORDFIELDPARSE_HPP
