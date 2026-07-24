#ifndef InfoRECORD_H
#define InfoRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct InfoRecord {
    QString editorId;
    quint32 formId;
    quint32 flags;
    QString responseText;
    QString voiceFile;
    QVector<quint32> conditionIds;
    quint32 targetId;
    QVector<quint32> scriptIds;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
};

inline bool operator==(const InfoRecord& l, const InfoRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.responseText == r.responseText && l.voiceFile == r.voiceFile
        && l.conditionIds == r.conditionIds && l.targetId == r.targetId
        && l.scriptIds == r.scriptIds && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const InfoRecord& l, const InfoRecord& r)
{
    return !(l == r);
}
#endif
