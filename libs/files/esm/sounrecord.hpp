#ifndef SOUNRECORD_HPP
#define SOUNRECORD_HPP

#include "records.hpp"
#include "variant.hpp"
#include <QString>
#include <QVector>

class ESMReader;
class ESMWriter;

struct SounRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString soundFile;
    QVector<RawSubRecord> rawSubRecords;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const SounRecord& l, const SounRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.flags == r.flags && l.soundFile == r.soundFile
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const SounRecord& l, const SounRecord& r)
{
    return !(l == r);
}

#endif // SOUNRECORD_HPP
