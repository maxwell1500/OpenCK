#ifndef QuestRECORD_H
#define QuestRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct QuestRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString questName;
    QString questDesc;
    quint32 questType = 0;
    QVector<quint32> stageIds;
    QVector<QString> stageDescriptions;
    QVector<quint32> objectiveIds;
    QVector<quint32> aliasIds;
    QString dialogueView;
    QVector<quint32> scriptIds;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const QuestRecord& l, const QuestRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.questName == r.questName && l.questDesc == r.questDesc
        && l.questType == r.questType && l.stageIds == r.stageIds
        && l.stageDescriptions == r.stageDescriptions && l.objectiveIds == r.objectiveIds
        && l.aliasIds == r.aliasIds && l.dialogueView == r.dialogueView
        && l.scriptIds == r.scriptIds && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const QuestRecord& l, const QuestRecord& r)
{
    return !(l == r);
}
#endif
