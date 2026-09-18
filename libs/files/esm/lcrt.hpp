#ifndef LCRT_H
#define LCRT_H

class ESMReader;
class ESMWriter;

#include "common.hpp"
#include "records.hpp"

#include <QString>
#include <QVector>

#include <memory>

struct LocationRefType
{
    quint32 formId = 0;
    QString editorId;
    Color color;
    QVector<RawSubRecord> rawSubRecords;

    // Verbatim round-trip (see src/model/world/verbatimrecord.hpp).
    QByteArray verbatimBody;
    quint32 verbatimFlags = 0;
    std::shared_ptr<LocationRefType> verbatimSnapshot;
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
