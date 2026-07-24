#ifndef DialRECORD_H
#define DialRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct DialRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString topicName;
    QVector<quint32> responseIds;
    QVector<quint32> conditionIds;
    QVector<quint32> animationIds;
    QVector<quint32> emotionIds;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const DialRecord& l, const DialRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.topicName == r.topicName && l.responseIds == r.responseIds
        && l.conditionIds == r.conditionIds && l.animationIds == r.animationIds
        && l.emotionIds == r.emotionIds && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const DialRecord& l, const DialRecord& r)
{
    return !(l == r);
}
#endif
