#ifndef EnchRECORD_H
#define EnchRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct EnchRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QVector<RawSubRecord> rawSubRecords;
    QString name;
    quint32 costLimit = 0;
    quint32 charges = 0;
    quint32 enchantmentData = 0;
    float charge = 0.0f;
    quint32 duration = 0;
    float magnitude = 0.0f;
    quint32 type = 0;
    quint32 soulGem = 0;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const EnchRecord& l, const EnchRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.rawSubRecords == r.rawSubRecords && l.name == r.name
        && l.costLimit == r.costLimit && l.charges == r.charges
        && l.enchantmentData == r.enchantmentData && l.charge == r.charge
        && l.duration == r.duration && l.magnitude == r.magnitude
        && l.type == r.type && l.soulGem == r.soulGem;
}

inline bool operator!=(const EnchRecord& l, const EnchRecord& r)
{
    return !(l == r);
}
#endif
