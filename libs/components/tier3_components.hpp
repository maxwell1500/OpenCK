#ifndef TIER3_COMPONENTS_HPP
#define TIER3_COMPONENTS_HPP

#include "component.hpp"
#include "editorproperty.hpp"
#include "../files/esm/esmreader.hpp"

#include <QString>
#include <QVector>

#include <memory>
#include <vector>

class ESMReader;
class ESMWriter;
struct RawSubRecord;

namespace tescomponents {

// ---------------------------------------------------------------------------
// TESFlags_Component — handles FNAM/FLAG subrecord as a uint32 bitfield.
// Used by RACE, DIAL, LOCATION, PACKAGE, SOUN, WTHR, LAND, and others.
// Starfield writes FNAM; older games write FLAG. We accept both.
// ---------------------------------------------------------------------------
/// Generic uint32 flags bitfield (FNAM/FLAG subrecord) for a record.
class TESFlags_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    quint32 flags = 0;
    std::vector<BitfieldDef> bitDefs;

    void setBitDefs(std::vector<BitfieldDef> defs) { bitDefs = std::move(defs); }

    QString name() const override { return QStringLiteral("Flags"); }
    QString className() const override { return QStringLiteral("TESFlags"); }
    static QString staticClassName() { return QStringLiteral("TESFlags"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('FNAM')
            || subrecordName == NAME('FLAG');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        if (subrecordName == NAME('FNAM') || subrecordName == NAME('FLAG'))
        {
            flags = esm.readType<quint32>();
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (flags != 0)
        {
            esm.writeSubData<quint32>(NAME('FNAM'), flags);
        }
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        if (!bitDefs.empty())
        {
            out.push_back(std::make_unique<BitfieldEditorProperty>(
                QStringLiteral("Flags"), &flags, bitDefs));
        }
        else
        {
            out.push_back(std::make_unique<UIntEditorProperty>(
                QStringLiteral("Flags"), &flags));
        }
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<TESFlags_Component>();
        c->flags = flags;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        flags = static_cast<const TESFlags_Component*>(other)->flags;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        return flags == static_cast<const TESFlags_Component*>(other)->flags;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

// ---------------------------------------------------------------------------
// BGSSoundDescriptor_Component — sound descriptor for SOUN records.
// Handles FNAM (sound file path) and SNDD/SNDX (flags).
// ---------------------------------------------------------------------------
/// Sound file path and flags (FNAM/SNDD/SNDX subrecords) for SOUN records.
class BGSSoundDescriptor_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    QString soundFile;
    quint32 soundFlags = 0;

    QString name() const override { return QStringLiteral("Sound Descriptor"); }
    QString className() const override { return QStringLiteral("BGSSoundDescriptor"); }
    static QString staticClassName() { return QStringLiteral("BGSSoundDescriptor"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('FNAM')
            || subrecordName == NAME('SNDD')
            || subrecordName == NAME('SNDX');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        if (subrecordName == NAME('FNAM'))
        {
            soundFile = esm.readZString();
        }
        else if (subrecordName == NAME('SNDD') || subrecordName == NAME('SNDX'))
        {
            soundFlags = esm.readType<quint32>();
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (!soundFile.isEmpty())
        {
            esm.writeSubZString(NAME('FNAM'), soundFile);
        }
        if (soundFlags != 0)
        {
            esm.writeSubData<quint32>(NAME('SNDX'), soundFlags);
        }
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<StringEditorProperty>(
            QStringLiteral("Sound File"), &soundFile));
        out.push_back(std::make_unique<UIntEditorProperty>(
            QStringLiteral("Sound Flags"), &soundFlags));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<BGSSoundDescriptor_Component>();
        c->soundFile = soundFile;
        c->soundFlags = soundFlags;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const BGSSoundDescriptor_Component*>(other);
        soundFile = o->soundFile;
        soundFlags = o->soundFlags;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const BGSSoundDescriptor_Component*>(other);
        return soundFile == o->soundFile && soundFlags == o->soundFlags;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

// ---------------------------------------------------------------------------
// TESWeatherData_Component — weather data for WTHR records.
// Handles SNAM (sun texture) and FNAM/FLAG (flags).
// ---------------------------------------------------------------------------
/// Weather sun texture and flags (SNAM/FNAM/FLAG subrecords) for WTHR records.
class TESWeatherData_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    QString sunTexture;
    quint32 weatherFlags = 0;

    QString name() const override { return QStringLiteral("Weather Data"); }
    QString className() const override { return QStringLiteral("TESWeatherData"); }
    static QString staticClassName() { return QStringLiteral("TESWeatherData"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('SNAM')
            || subrecordName == NAME('FNAM')
            || subrecordName == NAME('FLAG');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        if (subrecordName == NAME('SNAM'))
        {
            sunTexture = esm.readZString();
        }
        else if (subrecordName == NAME('FNAM') || subrecordName == NAME('FLAG'))
        {
            weatherFlags = esm.readType<quint32>();
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (!sunTexture.isEmpty())
        {
            esm.writeSubZString(NAME('SNAM'), sunTexture);
        }
        if (weatherFlags != 0)
        {
            esm.writeSubData<quint32>(NAME('FNAM'), weatherFlags);
        }
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<StringEditorProperty>(
            QStringLiteral("Sun Texture"), &sunTexture));
        out.push_back(std::make_unique<UIntEditorProperty>(
            QStringLiteral("Weather Flags"), &weatherFlags));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<TESWeatherData_Component>();
        c->sunTexture = sunTexture;
        c->weatherFlags = weatherFlags;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const TESWeatherData_Component*>(other);
        sunTexture = o->sunTexture;
        weatherFlags = o->weatherFlags;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const TESWeatherData_Component*>(other);
        return sunTexture == o->sunTexture && weatherFlags == o->weatherFlags;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

// ---------------------------------------------------------------------------
// BGSRefData_Component — reference position/rotation/scale for REFR records.
// Handles NAME (baseId), DATA (pos/rot/scale), XOWN (owner), DNAM (lock),
// XESP (initially disabled), SCRI (scripts).
// ---------------------------------------------------------------------------
/// Reference placement, owner, lock, and script data for REFR records.
class BGSRefData_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    quint32 baseId = 0;
    float posX = 0, posY = 0, posZ = 0;
    float rotX = 0, rotY = 0, rotZ = 0;
    float scale = 1.0f;
    quint32 owner = 0;
    quint32 lockLevel = 0;
    bool initiallyDisabled = false;
    QVector<quint32> scriptIds;

    QString name() const override { return QStringLiteral("Reference Data"); }
    QString className() const override { return QStringLiteral("BGSRefData"); }
    static QString staticClassName() { return QStringLiteral("BGSRefData"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('NAME')
            || subrecordName == NAME('DATA')
            || subrecordName == NAME('XSCL')
            || subrecordName == NAME('XOWN')
            || subrecordName == NAME('DNAM')
            || subrecordName == NAME('XESP')
            || subrecordName == NAME('SCRI');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        switch (subrecordName)
        {
        case NAME('NAME'):
            baseId = esm.readType<quint32>();
            break;
        case NAME('DATA'):
            posX = esm.readType<float>();
            posY = esm.readType<float>();
            posZ = esm.readType<float>();
            rotX = esm.readType<float>();
            rotY = esm.readType<float>();
            rotZ = esm.readType<float>();
            // Starfield/Skyrim DATA is 24 bytes (no scale); some legacy
            // records carry a 7th float. Only read it when it is present.
            if (esm.subLeft() >= static_cast<qint64>(sizeof(float)))
                scale = esm.readType<float>();
            break;
        case NAME('XSCL'):
            scale = esm.readType<float>();
            break;
        case NAME('XOWN'):
            owner = esm.readType<quint32>();
            break;
        case NAME('DNAM'):
            lockLevel = esm.readType<quint32>();
            break;
        case NAME('XESP'):
            initiallyDisabled = (esm.readType<quint32>() != 0);
            break;
        case NAME('SCRI'):
        {
            qint64 n = esm.subLeft() / 4;
            scriptIds.clear();
            scriptIds.reserve(n);
            for (qint64 i = 0; i < n; ++i)
                scriptIds.append(esm.readType<quint32>());
            break;
        }
        default:
            break;
        }
    }

    void save(ESMWriter& esm) const override
    {
        esm.writeSubData<quint32>(NAME('NAME'), baseId);
        esm.startSubRecord(NAME('DATA'));
        esm.writeType<float>(posX);
        esm.writeType<float>(posY);
        esm.writeType<float>(posZ);
        esm.writeType<float>(rotX);
        esm.writeType<float>(rotY);
        esm.writeType<float>(rotZ);
        esm.endSubRecord();
        if (scale != 1.0f)
            esm.writeSubData<float>(NAME('XSCL'), scale);
        if (owner != 0)
            esm.writeSubData<quint32>(NAME('XOWN'), owner);
        if (lockLevel != 0)
            esm.writeSubData<quint32>(NAME('DNAM'), lockLevel);
        if (initiallyDisabled)
            esm.writeSubData<quint32>(NAME('XESP'), 1);
        if (!scriptIds.isEmpty())
        {
            esm.startSubRecord(NAME('SCRI'));
            for (quint32 id : scriptIds)
                esm.writeType<quint32>(id);
            esm.endSubRecord();
        }
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<FormEditorProperty>(
            QStringLiteral("Base Object"), &baseId));
        out.push_back(std::make_unique<FloatEditorProperty>(
            QStringLiteral("Position X"), &posX));
        out.push_back(std::make_unique<FloatEditorProperty>(
            QStringLiteral("Position Y"), &posY));
        out.push_back(std::make_unique<FloatEditorProperty>(
            QStringLiteral("Position Z"), &posZ));
        out.push_back(std::make_unique<FloatEditorProperty>(
            QStringLiteral("Rotation X"), &rotX));
        out.push_back(std::make_unique<FloatEditorProperty>(
            QStringLiteral("Rotation Y"), &rotY));
        out.push_back(std::make_unique<FloatEditorProperty>(
            QStringLiteral("Rotation Z"), &rotZ));
        out.push_back(std::make_unique<FloatEditorProperty>(
            QStringLiteral("Scale"), &scale));
        out.push_back(std::make_unique<FormEditorProperty>(
            QStringLiteral("Owner"), &owner));
        out.push_back(std::make_unique<UIntEditorProperty>(
            QStringLiteral("Lock Level"), &lockLevel));
        out.push_back(std::make_unique<BoolEditorProperty>(
            QStringLiteral("Initially Disabled"), &initiallyDisabled));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<BGSRefData_Component>();
        c->baseId = baseId;
        c->posX = posX; c->posY = posY; c->posZ = posZ;
        c->rotX = rotX; c->rotY = rotY; c->rotZ = rotZ;
        c->scale = scale;
        c->owner = owner;
        c->lockLevel = lockLevel;
        c->initiallyDisabled = initiallyDisabled;
        c->scriptIds = scriptIds;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const BGSRefData_Component*>(other);
        baseId = o->baseId;
        posX = o->posX; posY = o->posY; posZ = o->posZ;
        rotX = o->rotX; rotY = o->rotY; rotZ = o->rotZ;
        scale = o->scale;
        owner = o->owner;
        lockLevel = o->lockLevel;
        initiallyDisabled = o->initiallyDisabled;
        scriptIds = o->scriptIds;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const BGSRefData_Component*>(other);
        return baseId == o->baseId && posX == o->posX && posY == o->posY && posZ == o->posZ
            && rotX == o->rotX && rotY == o->rotY && rotZ == o->rotZ && scale == o->scale
            && owner == o->owner && lockLevel == o->lockLevel
            && initiallyDisabled == o->initiallyDisabled && scriptIds == o->scriptIds;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

// ---------------------------------------------------------------------------
// TESActorBaseData_Component — NPC/creature base data from the ACBS
// subrecord. Handles flags, base spell, fatigue, barter gold, level,
// calc min/max, and speed multiplier.
// ---------------------------------------------------------------------------
/// NPC/creature base stats (ACBS subrecord): flags, level, spells, gold.
class TESActorBaseData_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    quint32 flags = 0;
    quint16 baseSpell = 0;
    quint16 fatigue = 0;
    quint16 barterGold = 0;
    qint16 level = 0;
    quint16 calcMin = 0;
    quint16 calcMax = 0;
    quint16 speedMult = 0;

    QString name() const override { return QStringLiteral("Actor Base Data"); }
    QString className() const override { return QStringLiteral("TESActorBaseData"); }
    static QString staticClassName() { return QStringLiteral("TESActorBaseData"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('ACBS');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        if (subrecordName == NAME('ACBS'))
        {
            flags = esm.readType<quint32>();
            baseSpell = esm.readType<quint16>();
            fatigue = esm.readType<quint16>();
            barterGold = esm.readType<quint16>();
            level = esm.readType<qint16>();
            calcMin = esm.readType<quint16>();
            calcMax = esm.readType<quint16>();
            speedMult = esm.readType<quint16>();
        }
    }

    void save(ESMWriter& esm) const override
    {
        esm.startSubRecord(NAME('ACBS'));
        esm.writeType<quint32>(flags);
        esm.writeType<quint16>(baseSpell);
        esm.writeType<quint16>(fatigue);
        esm.writeType<quint16>(barterGold);
        esm.writeType<qint16>(level);
        esm.writeType<quint16>(calcMin);
        esm.writeType<quint16>(calcMax);
        esm.writeType<quint16>(speedMult);
        esm.endSubRecord();
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<UIntEditorProperty>(
            QStringLiteral("Flags"), &flags));
        out.push_back(std::make_unique<IntEditorProperty>(
            QStringLiteral("Base Spell"), reinterpret_cast<qint32*>(&baseSpell)));
        out.push_back(std::make_unique<IntEditorProperty>(
            QStringLiteral("Fatigue"), reinterpret_cast<qint32*>(&fatigue)));
        out.push_back(std::make_unique<IntEditorProperty>(
            QStringLiteral("Barter Gold"), reinterpret_cast<qint32*>(&barterGold)));
        out.push_back(std::make_unique<IntEditorProperty>(
            QStringLiteral("Level"), reinterpret_cast<qint32*>(&level)));
        out.push_back(std::make_unique<IntEditorProperty>(
            QStringLiteral("Calc Min"), reinterpret_cast<qint32*>(&calcMin)));
        out.push_back(std::make_unique<IntEditorProperty>(
            QStringLiteral("Calc Max"), reinterpret_cast<qint32*>(&calcMax)));
        out.push_back(std::make_unique<IntEditorProperty>(
            QStringLiteral("Speed Mult"), reinterpret_cast<qint32*>(&speedMult)));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<TESActorBaseData_Component>();
        c->flags = flags; c->baseSpell = baseSpell; c->fatigue = fatigue;
        c->barterGold = barterGold; c->level = level;
        c->calcMin = calcMin; c->calcMax = calcMax; c->speedMult = speedMult;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const TESActorBaseData_Component*>(other);
        flags = o->flags; baseSpell = o->baseSpell; fatigue = o->fatigue;
        barterGold = o->barterGold; level = o->level;
        calcMin = o->calcMin; calcMax = o->calcMax; speedMult = o->speedMult;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const TESActorBaseData_Component*>(other);
        return flags == o->flags && baseSpell == o->baseSpell
            && fatigue == o->fatigue && barterGold == o->barterGold
            && level == o->level && calcMin == o->calcMin
            && calcMax == o->calcMax && speedMult == o->speedMult;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

// ---------------------------------------------------------------------------
// TESSpellList_Component — a counted-array SPLO subrecord containing
// form IDs of spells known by an NPC or actor. Used by NPC_ and CREA
// records.
// ---------------------------------------------------------------------------
/// List of spell form IDs (SPLO subrecord) known by an NPC or creature.
class TESSpellList_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    QVector<quint32> spells;

    QString name() const override { return QStringLiteral("Spell List"); }
    QString className() const override { return QStringLiteral("TESSpellList"); }
    static QString staticClassName() { return QStringLiteral("TESSpellList"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('SPLO');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        if (subrecordName == NAME('SPLO'))
        {
            // Skyrim/CREA write one SPLO per spell (4 bytes each).
            // Append per subrecord; the vector is cleared at record
            // init (initComponents() constructs fresh components).
            qint64 count = esm.subLeft() / 4;
            for (qint64 i = 0; i < count; ++i)
                spells.append(esm.readType<quint32>());
        }
    }

    void save(ESMWriter& esm) const override
    {
        for (quint32 id : spells)
            esm.writeSubData<quint32>(NAME('SPLO'), id);
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<FormArrayEditorProperty>(
            QStringLiteral("Spells"), &spells));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<TESSpellList_Component>();
        c->spells = spells;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        spells = static_cast<const TESSpellList_Component*>(other)->spells;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        return spells == static_cast<const TESSpellList_Component*>(other)->spells;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

// ---------------------------------------------------------------------------
// TESBodyParts_Component — handles BODT/BOD2 (body part data) subrecords.
// BODT is the legacy format (12 bytes: partType + flags). BOD2 is the newer
// format (12 bytes: partType + flags + partCount).
// ---------------------------------------------------------------------------
/// Body part type, flags, and count (BODT/BOD2 subrecords) for RACE records.
class TESBodyParts_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    quint32 partType = 0;
    quint32 flags = 0;
    quint32 partCount = 0;

    QString name() const override { return QStringLiteral("Body Data"); }
    QString className() const override { return QStringLiteral("TESBodyParts"); }
    static QString staticClassName() { return QStringLiteral("TESBodyParts"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('BODT')
            || subrecordName == NAME('BOD2');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        if (subrecordName == NAME('BODT'))
        {
            partType = esm.readType<quint32>();
            flags = esm.readType<quint32>();
        }
        else if (subrecordName == NAME('BOD2'))
        {
            partType = esm.readType<quint32>();
            flags = esm.readType<quint32>();
            partCount = esm.readType<quint32>();
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (partCount == 0)
        {
            esm.startSubRecord(NAME('BODT'));
            esm.writeType<quint32>(partType);
            esm.writeType<quint32>(flags);
            esm.endSubRecord();
        }
        else
        {
            esm.startSubRecord(NAME('BOD2'));
            esm.writeType<quint32>(partType);
            esm.writeType<quint32>(flags);
            esm.writeType<quint32>(partCount);
            esm.endSubRecord();
        }
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<UIntEditorProperty>(QStringLiteral("Part Type"), &partType));
        out.push_back(std::make_unique<UIntEditorProperty>(QStringLiteral("Body Flags"), &flags));
        out.push_back(std::make_unique<UIntEditorProperty>(QStringLiteral("Part Count"), &partCount));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<TESBodyParts_Component>();
        c->partType = partType; c->flags = flags; c->partCount = partCount;
        return c;
    }
    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const TESBodyParts_Component*>(other);
        partType = o->partType; flags = o->flags; partCount = o->partCount;
    }
    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const TESBodyParts_Component*>(other);
        return partType == o->partType && flags == o->flags && partCount == o->partCount;
    }
    void mergeWith(const Component* other) override { copyFrom(other); }
};

// ---------------------------------------------------------------------------
// TESAIForm_Component — AI data (AIDT subrecord) for NPC_ and CREA records.
// Stores aggression, confidence, energy, morality, mood, and disposition.
// ---------------------------------------------------------------------------
/// AI personality data (AIDT subrecord): aggression, confidence, morality.
class TESAIForm_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    quint8 aggression = 0;
    quint8 confidence = 0;
    quint8 energy = 0;
    quint8 morality = 0;
    qint16 mood = 0;
    quint8 moodSpeed = 0;
    quint8 disposition = 0;
    quint8 aggressionLevel = 0;

    QString name() const override { return QStringLiteral("AI Data"); }
    QString className() const override { return QStringLiteral("TESAIForm"); }
    static QString staticClassName() { return QStringLiteral("TESAIForm"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('AIDT');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        if (subrecordName == NAME('AIDT'))
        {
            aggression = esm.readType<quint8>();
            confidence = esm.readType<quint8>();
            energy = esm.readType<quint8>();
            morality = esm.readType<quint8>();
            mood = esm.readType<qint16>();
            moodSpeed = esm.readType<quint8>();
            disposition = esm.readType<quint8>();
            aggressionLevel = esm.readType<quint8>();
            esm.skip(2); // padding
        }
    }

    void save(ESMWriter& esm) const override
    {
        esm.startSubRecord(NAME('AIDT'));
        esm.writeType<quint8>(aggression);
        esm.writeType<quint8>(confidence);
        esm.writeType<quint8>(energy);
        esm.writeType<quint8>(morality);
        esm.writeType<qint16>(mood);
        esm.writeType<quint8>(moodSpeed);
        esm.writeType<quint8>(disposition);
        esm.writeType<quint8>(aggressionLevel);
        static const quint8 padding[2] = {0, 0};
        esm.writeRawData(reinterpret_cast<const char*>(padding), 2);
        esm.endSubRecord();
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<EnumEditorProperty>(QStringLiteral("Aggression"), reinterpret_cast<quint32*>(&aggression),
            std::vector<EnumEditorProperty::Entry>{
                {"Unaggressive", 0}, {"Aggressive", 1}, {"Very Aggressive", 2},
                {"Frenzied", 3}, {"Defensive", 4}, {"Cowardly", 5}}));
        out.push_back(std::make_unique<EnumEditorProperty>(QStringLiteral("Confidence"), reinterpret_cast<quint32*>(&confidence),
            std::vector<EnumEditorProperty::Entry>{
                {"Cowardly", 0}, {"Cautious", 1}, {"Average", 2},
                {"Brave", 3}, {"Foolhardy", 4}, {"Berserk", 5}}));
        out.push_back(std::make_unique<EnumEditorProperty>(QStringLiteral("Morality"), reinterpret_cast<quint32*>(&morality),
            std::vector<EnumEditorProperty::Entry>{
                {"Any", 0}, {"Low", 1}, {"Standard", 2},
                {"High", 3}, {"None", 4}}));
        out.push_back(std::make_unique<IntEditorProperty>(QStringLiteral("Energy"), reinterpret_cast<qint32*>(&energy)));
        out.push_back(std::make_unique<IntEditorProperty>(QStringLiteral("Mood"), reinterpret_cast<qint32*>(&mood)));
        out.push_back(std::make_unique<IntEditorProperty>(QStringLiteral("Mood Speed"), reinterpret_cast<qint32*>(&moodSpeed)));
        out.push_back(std::make_unique<IntEditorProperty>(QStringLiteral("Disposition"), reinterpret_cast<qint32*>(&disposition)));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<TESAIForm_Component>();
        c->aggression = aggression; c->confidence = confidence;
        c->energy = energy; c->morality = morality; c->mood = mood;
        c->moodSpeed = moodSpeed; c->disposition = disposition;
        c->aggressionLevel = aggressionLevel;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const TESAIForm_Component*>(other);
        aggression = o->aggression; confidence = o->confidence;
        energy = o->energy; morality = o->morality; mood = o->mood;
        moodSpeed = o->moodSpeed; disposition = o->disposition;
        aggressionLevel = o->aggressionLevel;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const TESAIForm_Component*>(other);
        return aggression == o->aggression && confidence == o->confidence
            && energy == o->energy && morality == o->morality && mood == o->mood
            && moodSpeed == o->moodSpeed && disposition == o->disposition
            && aggressionLevel == o->aggressionLevel;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

/// Per-skill value array (SKIL subrecords) for NPC_ and RACE records.
class TESSkills_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    QVector<qint32> skillValues;
    QVector<RawSubRecord> rawSub;

    QString name() const override { return QStringLiteral("Skills"); }
    QString className() const override { return QStringLiteral("TESSkills"); }
    static QString staticClassName() { return QStringLiteral("TESSkills"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('SKIL');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        if (subrecordName == NAME('SKIL'))
        {
            quint32 skillId = esm.readType<quint32>();
            qint32 val = esm.readType<qint32>();
            if (skillId >= static_cast<quint32>(skillValues.size()))
                skillValues.resize(skillId + 1);
            skillValues[skillId] = val;
        }
    }

    void save(ESMWriter& esm) const override
    {
        for (int i = 0; i < skillValues.size(); ++i)
        {
            if (skillValues[i] == 0) continue;
            esm.startSubRecord(NAME('SKIL'));
            esm.writeType<quint32>(static_cast<quint32>(i));
            esm.writeType<qint32>(skillValues[i]);
            esm.endSubRecord();
        }
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        QStringList skillNames = {
            "Block", "Armorer", "Medium Armor", "Heavy Armor",
            "Blunt", "Long Blade", "Axe", "Spear",
            "Athletics", "Enchant", "Destruction", "Alteration",
            "Illusion", "Conjuration", "Mysticism", "Restoration",
            "Alchemy", "Unarmored", "Security", "Sneak",
            "Acrobatics", "Light Armor", "Short Blade", "Marksman",
            "Mercantile", "Speechcraft", "Hand-to-Hand"
        };
        for (int i = 0; i < skillValues.size() && i < skillNames.size(); ++i)
        {
            out.push_back(std::make_unique<IntEditorProperty>(
                skillNames[i], &skillValues[i]));
        }
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<TESSkills_Component>();
        c->skillValues = skillValues;
        return c;
    }
    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        skillValues = static_cast<const TESSkills_Component*>(other)->skillValues;
    }
    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        return skillValues == static_cast<const TESSkills_Component*>(other)->skillValues;
    }
    void mergeWith(const Component* other) override { copyFrom(other); }
};

// ---------------------------------------------------------------------------
// TESAttributes_Component — handles game-version-specific attribute data.
// Morrowind: multiple BYDT subrecords (one per attribute, ID+int32).
// Skyrim:    ATTR subrecord with 8 packed uint16 values.
// Fallout 4: SPECIAL (7 attributes) via ATTR.
// Starfield: ATTR subrecord.
// ---------------------------------------------------------------------------
/// Actor attribute array (ATTR/BYDT subrecords), format varies by game.
class TESAttributes_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    QVector<qint32> attributes;
    QVector<RawSubRecord> rawSub;

    QString name() const override { return QStringLiteral("Attributes"); }
    QString className() const override { return QStringLiteral("TESAttributes"); }
    static QString staticClassName() { return QStringLiteral("TESAttributes"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('ATTR')
            || subrecordName == NAME('BYDT');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        if (subrecordName == NAME('ATTR'))
        {
            qint64 count = esm.subLeft() / 2;
            attributes.clear();
            attributes.reserve(count);
            for (qint64 i = 0; i < count; ++i)
                attributes.append(esm.readType<qint16>());
        }
        else if (subrecordName == NAME('BYDT'))
        {
            quint8 attrId = esm.readType<quint8>();
            qint32 val = esm.readType<qint32>();
            if (attrId >= static_cast<quint32>(attributes.size()))
                attributes.resize(attrId + 1);
            attributes[attrId] = val;
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (attributes.isEmpty()) return;
        esm.startSubRecord(NAME('ATTR'));
        for (qint32 a : attributes)
            esm.writeType<qint16>(static_cast<qint16>(a));
        esm.endSubRecord();
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        QStringList attrNames = {
            "Strength", "Intelligence", "Willpower", "Agility",
            "Speed", "Endurance", "Personality", "Luck"
        };
        for (int i = 0; i < attributes.size() && i < attrNames.size(); ++i)
        {
            out.push_back(std::make_unique<IntEditorProperty>(
                attrNames[i], &attributes[i]));
        }
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<TESAttributes_Component>();
        c->attributes = attributes;
        return c;
    }
    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        attributes = static_cast<const TESAttributes_Component*>(other)->attributes;
    }
    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        return attributes == static_cast<const TESAttributes_Component*>(other)->attributes;
    }
    void mergeWith(const Component* other) override { copyFrom(other); }
};

// ---------------------------------------------------------------------------
// TESNPCFaceGen_Component — NPC face/head data across game versions.
// HNAM/ENAM (hair/eyes) in Morrowind/Skyrim, QNAM (face tint texture) in
// Skyrim/FO4, PNAM (head parts list) in Skyrim/FO4, NAMA/NAM9 (sym/asym
// face morph values) in Skyrim. Game-specific subrecords (FGGS, FGGA,
// FGTR, NIFT, ...) are preserved verbatim in rawSub.
// ---------------------------------------------------------------------------
/// NPC face generation data: hair, eyes, head parts, and face morphs.
class TESNPCFaceGen_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    quint32 hairFormId = 0;          // HNAM — hair form (Morrowind/Skyrim)
    quint32 eyesFormId = 0;          // ENAM — eyes form (Morrowind/Skyrim)
    quint32 faceTextureFormId = 0;   // QNAM — face tint texture (Skyrim/FO4)
    QVector<quint32> headParts;     // PNAM — head parts list (Skyrim/FO4)
    QVector<float> faceMorphSym;    // NAMA — symmetric face morph values (Skyrim)
    QVector<float> faceMorphAsym;   // NAM9 — asymmetric face morph values (Skyrim)
    QVector<RawSubRecord> rawSub;   // for unknown face gen subrecords

    QString name() const override { return QStringLiteral("Face Gen"); }
    QString className() const override { return QStringLiteral("TESNPCFaceGen"); }
    static QString staticClassName() { return QStringLiteral("TESNPCFaceGen"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('HNAM')
            || subrecordName == NAME('ENAM')
            || subrecordName == NAME('QNAM')
            || subrecordName == NAME('PNAM')
            || subrecordName == NAME('NAMA')
            || subrecordName == NAME('NAM9')
            || subrecordName == NAME('FGGS')
            || subrecordName == NAME('FGGA')
            || subrecordName == NAME('FGTR')
            || subrecordName == NAME('NIFT');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        switch (subrecordName)
        {
        case NAME('HNAM'):
            hairFormId = esm.readType<quint32>();
            break;
        case NAME('ENAM'):
            eyesFormId = esm.readType<quint32>();
            break;
        case NAME('QNAM'):
            faceTextureFormId = esm.readType<quint32>();
            break;
        case NAME('PNAM'):
        {
            // Skyrim writes one PNAM per head part (4 bytes each).
            // Append per subrecord; cleared at record init.
            qint64 n = esm.subLeft() / 4;
            for (qint64 i = 0; i < n; ++i)
                headParts.append(esm.readType<quint32>());
            break;
        }
        case NAME('NAMA'):
        {
            qint64 n = esm.subLeft() / 4;
            faceMorphSym.clear();
            faceMorphSym.reserve(n);
            for (qint64 i = 0; i < n; ++i)
                faceMorphSym.append(esm.readType<float>());
            break;
        }
        case NAME('NAM9'):
        {
            qint64 n = esm.subLeft() / 4;
            faceMorphAsym.clear();
            faceMorphAsym.reserve(n);
            for (qint64 i = 0; i < n; ++i)
                faceMorphAsym.append(esm.readType<float>());
            break;
        }
        default:
        {
            // Unknown face gen subrecord — preserve raw bytes
            RawSubRecord raw;
            raw.name = subrecordName;
            esm.readRawSubData(raw.data);
            rawSub.append(raw);
            break;
        }
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (hairFormId != 0)
            esm.writeSubData<quint32>(NAME('HNAM'), hairFormId);
        if (eyesFormId != 0)
            esm.writeSubData<quint32>(NAME('ENAM'), eyesFormId);
        if (faceTextureFormId != 0)
            esm.writeSubData<quint32>(NAME('QNAM'), faceTextureFormId);
        for (quint32 p : headParts)
            esm.writeSubData<quint32>(NAME('PNAM'), p);
        if (!faceMorphSym.isEmpty())
        {
            esm.startSubRecord(NAME('NAMA'));
            for (float v : faceMorphSym)
                esm.writeType<float>(v);
            esm.endSubRecord();
        }
        if (!faceMorphAsym.isEmpty())
        {
            esm.startSubRecord(NAME('NAM9'));
            for (float v : faceMorphAsym)
                esm.writeType<float>(v);
            esm.endSubRecord();
        }
        for (const auto& raw : rawSub)
        {
            esm.startSubRecord(raw.name);
            esm.writeRawData(raw.data.data(), raw.data.size());
            esm.endSubRecord();
        }
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<FormEditorProperty>(
            QStringLiteral("Hair"), &hairFormId));
        out.push_back(std::make_unique<FormEditorProperty>(
            QStringLiteral("Eyes"), &eyesFormId));
        out.push_back(std::make_unique<FormEditorProperty>(
            QStringLiteral("Face Texture"), &faceTextureFormId));
        out.push_back(std::make_unique<FormArrayEditorProperty>(
            QStringLiteral("Head Parts"), &headParts));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<TESNPCFaceGen_Component>();
        c->hairFormId = hairFormId;
        c->eyesFormId = eyesFormId;
        c->faceTextureFormId = faceTextureFormId;
        c->headParts = headParts;
        c->faceMorphSym = faceMorphSym;
        c->faceMorphAsym = faceMorphAsym;
        c->rawSub = rawSub;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const TESNPCFaceGen_Component*>(other);
        hairFormId = o->hairFormId;
        eyesFormId = o->eyesFormId;
        faceTextureFormId = o->faceTextureFormId;
        headParts = o->headParts;
        faceMorphSym = o->faceMorphSym;
        faceMorphAsym = o->faceMorphAsym;
        rawSub = o->rawSub;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const TESNPCFaceGen_Component*>(other);
        return hairFormId == o->hairFormId
            && eyesFormId == o->eyesFormId
            && faceTextureFormId == o->faceTextureFormId
            && headParts == o->headParts
            && faceMorphSym == o->faceMorphSym
            && faceMorphAsym == o->faceMorphAsym
            && rawSub == o->rawSub;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

} // namespace tescomponents

#endif // TIER3_COMPONENTS_HPP
