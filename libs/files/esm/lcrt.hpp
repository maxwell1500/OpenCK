#ifndef LCRT_H
#define LCRT_H

class ESMReader;
class ESMWriter;

#include "common.hpp"
#include "records.hpp"

#include <QString>
#include <QVector>

struct LocationRefType
{
    QString editorId;
    Color color;
    QVector<RawSubRecord> rawSubRecords;

    void load(ESMReader& esm, bool base = false);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const LocationRefType& l, const LocationRefType& r)
{
    return l.editorId == r.editorId && l.color == r.color
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const LocationRefType& l, const LocationRefType& r)
{
    return !(l == r);
}

#endif // LCRT_H
