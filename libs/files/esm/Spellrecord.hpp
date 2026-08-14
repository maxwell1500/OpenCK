#ifndef SpellRECORD_H
#define SpellRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct SpellRecord {
    openck::FormComponents components;
    QString editorId;
    QString fullName;
    quint32 formId = 0;
    quint32 flags = 0;
    quint32 spellType = 0;
    quint32 cost = 0;
    quint32 castingSound = 0;
    QVector<quint32> effects;
    quint32 enchantment = 0;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const SpellRecord& l, const SpellRecord& r)
{
    return l.editorId == r.editorId && l.fullName == r.fullName && l.formId == r.formId && l.flags == r.flags
        && l.spellType == r.spellType && l.cost == r.cost && l.castingSound == r.castingSound
        && l.effects == r.effects && l.enchantment == r.enchantment
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const SpellRecord& l, const SpellRecord& r)
{
    return !(l == r);
}
#endif
