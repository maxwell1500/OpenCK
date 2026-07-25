#ifndef DialRECORD_H
#define DialRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include "../../components/tier3_components.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct DialRecord {
    openck::FormComponents components;
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
    void initComponents();
};

inline bool operator==(const DialRecord& l, const DialRecord& r)
{
    return l.components == r.components && l.editorId == r.editorId
        && l.formId == r.formId && l.flags == r.flags
        && l.topicName == r.topicName && l.responseIds == r.responseIds
        && l.conditionIds == r.conditionIds && l.animationIds == r.animationIds
        && l.emotionIds == r.emotionIds && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const DialRecord& l, const DialRecord& r)
{
    return !(l == r);
}
#endif
