#ifndef MATERIALRECORD_H
#define MATERIALRECORD_H

#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"

#include <QString>
#include <QVector>

class ESMReader;
class ESMWriter;

struct MaterialRecord
{
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    
    QString materialName;
    QString name;
    QString description;
    
    QString iconPath;
    QString modelPath;
    QString bnam;
    QString cnam;
    QString texturePath;
    
    quint32 materialType = 0;
    quint32 value = 0;
    quint32 weight = 0;
    quint32 health = 0;
    quint32 magicka = 0;
    quint32 stamina = 0;
    quint32 level = 0;
    quint32 race = 0;
    quint32 faction = 0;
    quint32 stage = 0;
    quint32 difficulty = 0;
    
    QVector<RawSubRecord> rawSubRecords;
    
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const MaterialRecord& l, const MaterialRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.flags == r.flags
        && l.materialName == r.materialName && l.name == r.name
        && l.description == r.description
        && l.iconPath == r.iconPath && l.modelPath == r.modelPath
        && l.bnam == r.bnam && l.cnam == r.cnam && l.texturePath == r.texturePath
        && l.materialType == r.materialType && l.value == r.value
        && l.weight == r.weight && l.health == r.health
        && l.magicka == r.magicka && l.stamina == r.stamina
        && l.level == r.level && l.race == r.race && l.faction == r.faction
        && l.stage == r.stage && l.difficulty == r.difficulty
        && l.components == r.components
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const MaterialRecord& l, const MaterialRecord& r)
{
    return !(l == r);
}

#endif // MATERIALRECORD_H
