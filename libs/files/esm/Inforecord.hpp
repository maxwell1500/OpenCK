#ifndef InfoRECORD_H
#define InfoRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVariant>
#include <QVector>
class ESMReader;
class ESMWriter;

struct DialogueCondition {
    QString function;
    QString comparison;
    QVariant value;
    bool useAND = true;

    bool operator==(const DialogueCondition& other) const
    {
        return function == other.function && comparison == other.comparison
            && value == other.value && useAND == other.useAND;
    }
};

struct InfoRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId;
    quint32 flags;
    QString responseText;
    QString voiceFile;
    QVector<quint32> conditionIds;
    QVector<DialogueCondition> conditions;
    QString scriptFragment;
    quint32 targetId;
    QVector<quint32> scriptIds;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const InfoRecord& l, const InfoRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.responseText == r.responseText && l.voiceFile == r.voiceFile
        && l.conditionIds == r.conditionIds && l.conditions == r.conditions
        && l.scriptFragment == r.scriptFragment && l.targetId == r.targetId
        && l.scriptIds == r.scriptIds && l.rawSubRecords == r.rawSubRecords
        && l.components == r.components;
}

inline bool operator!=(const InfoRecord& l, const InfoRecord& r)
{
    return !(l == r);
}
#endif
