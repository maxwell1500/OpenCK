#ifndef LCRT_H
#define LCRT_H

class ESMReader;
class ESMWriter;

#include "common.hpp"

#include <QString>

struct LocationRefType
{
    QString editorId;
    Color color;

    void load(ESMReader& esm, bool base = false);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const LocationRefType& l, const LocationRefType& r)
{
    return l.editorId == r.editorId && l.color == r.color;
}

inline bool operator!=(const LocationRefType& l, const LocationRefType& r)
{
    return !(l == r);
}

#endif // LCRT_H
