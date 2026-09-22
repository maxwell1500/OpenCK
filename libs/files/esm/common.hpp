#ifndef COMMON_H
#define COMMON_H

#include <QtGlobal>
#include <QString>

typedef uint32_t NAME;
typedef uint32_t Color;

inline NAME swapName(NAME name)
{
    return (name >> 24 |
           ((name << 8) & 0x00FF0000) |
           ((name >> 8) & 0x0000FF00) |
           name << 24);
}

inline QString nameToQString(NAME name)
{
    char buf[5] = { 0, 0, 0, 0, 0 };
    buf[0] = static_cast<char>((name >> 24) & 0xFF);
    buf[1] = static_cast<char>((name >> 16) & 0xFF);
    buf[2] = static_cast<char>((name >> 8) & 0xFF);
    buf[3] = static_cast<char>(name & 0xFF);
    return QString::fromLatin1(buf);
}

#endif // COMMON_H
