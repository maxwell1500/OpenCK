#ifndef QuestRECORD_H
#define QuestRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>

#include <memory>
class ESMReader;
class ESMWriter;
struct QuestRecord {
    openck::FormComponents components;
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
    QVector<quint8> stageFlags;
    QVector<quint32> objectiveFlags;
    QVector<quint32> aliasFlags;
    QVector<QVector<RawSubRecord>> stageExtra;
    QVector<QVector<RawSubRecord>> objectiveExtra;
    QVector<QVector<RawSubRecord>> aliasExtra;
    QVector<RawSubRecord> rawSubRecords;
    // Positional replay: every subrecord's name plus how to re-emit it, in
    // load order. The action freezes the load-time routing decision, so save
    // is a dumb replay (no group tracking needed) and interleaved stage /
    // objective / alias / top-level runs round-trip exactly.
    QVector<NAME> loadOrder;
    QVector<quint8> loadAction;
    QVector<int> loadIndex;
    QVector<quint8> stageHasQsdt;
    QVector<quint8> stageHasCnam;
    bool hasEdid = false;
    // Per-occurrence QSDT/CNAM values: a stage may carry several of each
    // (script-result extras); the flat stageFlags/stageDescriptions mirrors
    // hold the last occurrence for the editors, while replay emits every
    // stored occurrence and the mirror at the last position.
    QVector<QVector<quint8>> stageQsdtVals;
    QVector<QVector<QString>> stageCnamVals;
    // Raw CNAM/desc/view payloads: stage descriptions and the top-level
    // description/view are binary or LString indices in shipped files, not
    // text — unedited occurrences replay these bytes verbatim.
    QVector<QVector<QByteArray>> stageCnamRaws;
    QByteArray questDescRaw;
    QByteArray dialogueViewRaw;
    // Verbatim round-trip (see src/model/world/verbatimrecord.hpp).
    QByteArray verbatimBody;
    quint32 verbatimFlags = 0;
    std::shared_ptr<QuestRecord> verbatimSnapshot;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

// QuestRecord replay actions, one per loadOrder entry (see loadAction).
enum QuestReplayAction : quint8
{
    QRA_Edid = 0,
    QRA_Component = 1,
    QRA_TopRaw = 2,
    QRA_StageIndex = 3,
    QRA_StageQsdt = 4,
    QRA_StageCnam = 5,
    QRA_Objective = 6,
    QRA_Alias = 7,
    QRA_StageExtra = 8,
    QRA_ObjectiveExtra = 9,
    QRA_AliasExtra = 10,
    QRA_QuestDesc = 11,
    QRA_DialogueView = 12
};

inline bool operator==(const QuestRecord& l, const QuestRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.questName == r.questName && l.questDesc == r.questDesc
        && l.questType == r.questType && l.stageIds == r.stageIds
        && l.stageDescriptions == r.stageDescriptions && l.objectiveIds == r.objectiveIds
        && l.aliasIds == r.aliasIds && l.dialogueView == r.dialogueView
        && l.scriptIds == r.scriptIds && l.components == r.components
        && l.stageFlags == r.stageFlags && l.objectiveFlags == r.objectiveFlags
        && l.aliasFlags == r.aliasFlags
        && l.stageExtra == r.stageExtra && l.objectiveExtra == r.objectiveExtra
        && l.aliasExtra == r.aliasExtra && l.rawSubRecords == r.rawSubRecords
        && l.loadOrder == r.loadOrder && l.loadAction == r.loadAction
        && l.loadIndex == r.loadIndex && l.stageHasQsdt == r.stageHasQsdt
        && l.stageHasCnam == r.stageHasCnam && l.hasEdid == r.hasEdid
        && l.stageQsdtVals == r.stageQsdtVals && l.stageCnamVals == r.stageCnamVals
        && l.stageCnamRaws == r.stageCnamRaws
        && l.questDescRaw == r.questDescRaw && l.dialogueViewRaw == r.dialogueViewRaw;
}

inline bool operator!=(const QuestRecord& l, const QuestRecord& r)
{
    return !(l == r);
}
#endif
