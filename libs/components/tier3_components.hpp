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
    bool hasFlags = false;
    quint8 flagsWidth = 4;
    QByteArray flagsExtra;
    // Every occurrence payload (FNAM/FLAG repeat; the fields above track the
    // last, and non-last occurrences replay from here).
    QVector<QByteArray> flagsRaws;
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
            QByteArray raw;
            esm.readRawSubData(raw);
            flagsRaws.append(raw);
            flagsExtra.clear();
            if (raw.size() >= 4)
            {
                quint32 v = 0;
                for (int i = 0; i < 4; ++i)
                    v |= quint32(static_cast<quint8>(raw.at(i))) << (8 * i);
                flags = v;
                flagsWidth = 4;
                if (raw.size() > 4)
                    flagsExtra = raw.mid(4);
            }
            else
            {
                flags = 0;
                flagsWidth = static_cast<quint8>(raw.size());
                for (int i = 0; i < raw.size(); ++i)
                    flags |= quint32(static_cast<quint8>(raw.at(i))) << (8 * i);
            }
            hasFlags = true;
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (hasFlags || flags != 0)
        {
            esm.startSubRecord(NAME('FNAM'));
            // Preserve a zero-width source exactly (empty FNAM stays empty);
            // only widen to the full u32 when flags were set on a widthless
            // record, so edited bits are never truncated to one byte.
            quint8 w = flagsWidth;
            if (w == 0 && flags != 0)
                w = 4;
            for (quint8 i = 0; i < w; ++i)
                esm.writeType<quint8>(static_cast<quint8>((flags >> (8 * i)) & 0xFF));
            if (!flagsExtra.isEmpty())
                esm.writeRawData(flagsExtra.constData(), flagsExtra.size());
            esm.endSubRecord();
        }
    }

    bool writeSubrecord(NAME subrecordName, ESMWriter& esm) const override
    {
        if (subrecordName != NAME('FNAM') && subrecordName != NAME('FLAG'))
            return false;
        if (hasFlags || flags != 0)
        {
            esm.startSubRecord(subrecordName);
            quint8 w = flagsWidth;
            if (w == 0 && flags != 0)
                w = 4;
            for (quint8 i = 0; i < w; ++i)
                esm.writeType<quint8>(static_cast<quint8>((flags >> (8 * i)) & 0xFF));
            if (!flagsExtra.isEmpty())
                esm.writeRawData(flagsExtra.constData(), flagsExtra.size());
            esm.endSubRecord();
        }
        return true;
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
        c->hasFlags = hasFlags;
        c->flagsWidth = flagsWidth;
        c->flagsExtra = flagsExtra;
        c->flagsRaws = flagsRaws;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const TESFlags_Component*>(other);
        flags = o->flags;
        hasFlags = o->hasFlags;
        flagsWidth = o->flagsWidth;
        flagsExtra = o->flagsExtra;
        flagsRaws = o->flagsRaws;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const TESFlags_Component*>(other);
        return flags == o->flags && hasFlags == o->hasFlags
            && flagsWidth == o->flagsWidth && flagsExtra == o->flagsExtra
            && flagsRaws == o->flagsRaws;
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
    quint32 flagsSpelling = NAME('SNDX');
    bool hasFlags = false;

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
            flagsSpelling = subrecordName;
            hasFlags = true;
            soundFlags = esm.readType<quint32>();
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (!soundFile.isEmpty())
        {
            esm.writeSubZString(NAME('FNAM'), soundFile);
        }
        if (hasFlags || soundFlags != 0)
        {
            esm.writeSubData<quint32>(flagsSpelling, soundFlags);
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
        c->flagsSpelling = flagsSpelling;
        c->hasFlags = hasFlags;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const BGSSoundDescriptor_Component*>(other);
        soundFile = o->soundFile;
        soundFlags = o->soundFlags;
        flagsSpelling = o->flagsSpelling;
        hasFlags = o->hasFlags;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const BGSSoundDescriptor_Component*>(other);
        return soundFile == o->soundFile && soundFlags == o->soundFlags
            && flagsSpelling == o->flagsSpelling && hasFlags == o->hasFlags;
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
    quint32 flagsSpelling = NAME('FNAM');
    bool hasFlags = false;

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
            flagsSpelling = subrecordName;
            hasFlags = true;
            weatherFlags = esm.readType<quint32>();
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (!sunTexture.isEmpty())
        {
            esm.writeSubZString(NAME('SNAM'), sunTexture);
        }
        if (hasFlags || weatherFlags != 0)
        {
            esm.writeSubData<quint32>(flagsSpelling, weatherFlags);
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
        c->flagsSpelling = flagsSpelling;
        c->hasFlags = hasFlags;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const TESWeatherData_Component*>(other);
        sunTexture = o->sunTexture;
        weatherFlags = o->weatherFlags;
        flagsSpelling = o->flagsSpelling;
        hasFlags = o->hasFlags;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const TESWeatherData_Component*>(other);
        return sunTexture == o->sunTexture && weatherFlags == o->weatherFlags
            && flagsSpelling == o->flagsSpelling && hasFlags == o->hasFlags;
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
    // Trailing bytes of an XOWN subrecord wider than the owner FormID
    // (Starfield writes 12-byte XOWN); preserved so the width round-trips.
    QByteArray ownerExtra;
    quint32 lockLevel = 0;
    bool initiallyDisabled = false;
    // Trailing bytes of an XESP wider than the enable/disable flag.
    QByteArray xespExtra;
    QVector<quint32> scriptIds;
    // Presence + width so save never invents NAME/DATA the source lacked and
    // re-emits the exact float count (24B = 6, legacy 7th scale = 7).
    bool hasName = false;
    bool hasData = false;
    int dataFloats = 6;

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
            hasName = true;
            // Starfield writes some NAME subrecords shorter than 32 bits.
            // Read only declared bytes LE so the stream never overruns.
            if (esm.subLeft() < static_cast<qint64>(sizeof(quint32)))
            {
                quint32 v = 0;
                qint64 n = esm.subLeft();
                for (qint64 i = 0; i < n; ++i)
                    v |= quint32(esm.readType<quint8>()) << (8 * i);
                baseId = v;
            }
            else
                baseId = esm.readType<quint32>();
            break;
        case NAME('DATA'):
            // DATA is 6 floats (24B) plus an optional 7th scale float.
            // Short variants exist; never read past the declared size.
            hasData = true;
            dataFloats = 0;
            if (esm.subLeft() >= static_cast<qint64>(sizeof(float)))
            {
                posX = esm.readType<float>();
                ++dataFloats;
            }
            if (esm.subLeft() >= static_cast<qint64>(sizeof(float)))
            {
                posY = esm.readType<float>();
                ++dataFloats;
            }
            if (esm.subLeft() >= static_cast<qint64>(sizeof(float)))
            {
                posZ = esm.readType<float>();
                ++dataFloats;
            }
            if (esm.subLeft() >= static_cast<qint64>(sizeof(float)))
            {
                rotX = esm.readType<float>();
                ++dataFloats;
            }
            if (esm.subLeft() >= static_cast<qint64>(sizeof(float)))
            {
                rotY = esm.readType<float>();
                ++dataFloats;
            }
            if (esm.subLeft() >= static_cast<qint64>(sizeof(float)))
            {
                rotZ = esm.readType<float>();
                ++dataFloats;
            }
            // Starfield/Skyrim DATA is 24 bytes (no scale); some legacy
            // records carry a 7th float. Only read it when it is present.
            if (esm.subLeft() >= static_cast<qint64>(sizeof(float)))
            {
                scale = esm.readType<float>();
                ++dataFloats;
            }
            break;
        case NAME('XSCL'):
            if (esm.subLeft() >= static_cast<qint64>(sizeof(float)))
                scale = esm.readType<float>();
            break;
        case NAME('XOWN'):
            ownerExtra.clear();
            if (esm.subLeft() < static_cast<qint64>(sizeof(quint32)))
            {
                quint32 v = 0;
                qint64 n = esm.subLeft();
                for (qint64 i = 0; i < n; ++i)
                    v |= quint32(esm.readType<quint8>()) << (8 * i);
                owner = v;
            }
            else
            {
                owner = esm.readType<quint32>();
                if (esm.subLeft() > 0)
                    esm.readRawSubData(ownerExtra);
            }
            break;
        case NAME('DNAM'):
            if (esm.subLeft() < static_cast<qint64>(sizeof(quint32)))
            {
                quint32 v = 0;
                qint64 n = esm.subLeft();
                for (qint64 i = 0; i < n; ++i)
                    v |= quint32(esm.readType<quint8>()) << (8 * i);
                lockLevel = v;
            }
            else
                lockLevel = esm.readType<quint32>();
            break;
        case NAME('XESP'):
            // Some records carry a wider XESP than the enable/disable flag;
            // keep the whole payload so the width and unknown words survive.
            xespExtra.clear();
            esm.readRawSubData(xespExtra);
            {
                quint32 v = 0;
                const int n = qMin<int>(xespExtra.size(), 4);
                for (int i = 0; i < n; ++i)
                    v |= quint32(static_cast<quint8>(xespExtra.at(i))) << (8 * i);
                initiallyDisabled = (v != 0);
            }
            break;
        case NAME('SCRI'):
        {
            if (esm.subLeft() <= 0)
            {
                scriptIds.clear();
                break;
            }
            qint64 n = esm.subLeft() / 4;
            scriptIds.clear();
            if (n > 0)
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
        {
            esm.startSubRecord(NAME('XOWN'));
            esm.writeType<quint32>(owner);
            if (!ownerExtra.isEmpty())
                esm.writeRawData(ownerExtra.constData(), ownerExtra.size());
            esm.endSubRecord();
        }
        if (lockLevel != 0)
            esm.writeSubData<quint32>(NAME('DNAM'), lockLevel);
        if (initiallyDisabled || !xespExtra.isEmpty())
        {
            esm.startSubRecord(NAME('XESP'));
            if (!xespExtra.isEmpty())
                esm.writeRawData(xespExtra.constData(), xespExtra.size());
            else
                esm.writeType<quint32>(initiallyDisabled ? 1u : 0u);
            esm.endSubRecord();
        }
        if (!scriptIds.isEmpty())
        {
            esm.startSubRecord(NAME('SCRI'));
            for (quint32 id : scriptIds)
                esm.writeType<quint32>(id);
            esm.endSubRecord();
        }
    }

    // Writes a single named subrecord from the current values. Used by
    // RefrRecord::save to replay the load order positionally: with
    // fromLoad=true the subrecord was present on disk and is emitted from
    // values unconditionally; with fromLoad=false the save() new-value
    // conditionals apply. Returns false for unhandled names.
    bool saveSubrecord(ESMWriter& esm, quint32 subrecordName, bool fromLoad) const
    {
        switch (subrecordName)
        {
        case NAME('NAME'):
            if (!fromLoad && !hasName)
                return false;
            esm.writeSubData<quint32>(NAME('NAME'), baseId);
            return true;
        case NAME('DATA'):
            if (!fromLoad && !hasData)
                return false;
            esm.startSubRecord(NAME('DATA'));
            if (dataFloats > 0) esm.writeType<float>(posX);
            if (dataFloats > 1) esm.writeType<float>(posY);
            if (dataFloats > 2) esm.writeType<float>(posZ);
            if (dataFloats > 3) esm.writeType<float>(rotX);
            if (dataFloats > 4) esm.writeType<float>(rotY);
            if (dataFloats > 5) esm.writeType<float>(rotZ);
            if (dataFloats > 6) esm.writeType<float>(scale);
            esm.endSubRecord();
            return true;
        case NAME('XSCL'):
            if (fromLoad || scale != 1.0f)
                esm.writeSubData<float>(NAME('XSCL'), scale);
            else
                return false;
            return true;
        case NAME('XOWN'):
            if (fromLoad || owner != 0)
            {
                esm.startSubRecord(NAME('XOWN'));
                esm.writeType<quint32>(owner);
                if (!ownerExtra.isEmpty())
                    esm.writeRawData(ownerExtra.constData(), ownerExtra.size());
                esm.endSubRecord();
            }
            else
                return false;
            return true;
        case NAME('DNAM'):
            if (fromLoad || lockLevel != 0)
                esm.writeSubData<quint32>(NAME('DNAM'), lockLevel);
            else
                return false;
            return true;
        case NAME('XESP'):
            if (fromLoad || initiallyDisabled || !xespExtra.isEmpty())
            {
                esm.startSubRecord(NAME('XESP'));
                if (fromLoad && !xespExtra.isEmpty())
                    esm.writeRawData(xespExtra.constData(), xespExtra.size());
                else
                    esm.writeType<quint32>(initiallyDisabled ? 1u : 0u);
                esm.endSubRecord();
            }
            else
                return false;
            return true;
        case NAME('SCRI'):
            if (fromLoad || !scriptIds.isEmpty())
            {
                esm.startSubRecord(NAME('SCRI'));
                for (quint32 id : scriptIds)
                    esm.writeType<quint32>(id);
                esm.endSubRecord();
            }
            else
                return false;
            return true;
        default:
            return false;
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
        c->ownerExtra = ownerExtra;
        c->lockLevel = lockLevel;
        c->initiallyDisabled = initiallyDisabled;
        c->xespExtra = xespExtra;
        c->scriptIds = scriptIds;
        c->hasName = hasName;
        c->hasData = hasData;
        c->dataFloats = dataFloats;
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
        ownerExtra = o->ownerExtra;
        lockLevel = o->lockLevel;
        initiallyDisabled = o->initiallyDisabled;
        xespExtra = o->xespExtra;
        scriptIds = o->scriptIds;
        hasName = o->hasName;
        hasData = o->hasData;
        dataFloats = o->dataFloats;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const BGSRefData_Component*>(other);
        return baseId == o->baseId && posX == o->posX && posY == o->posY && posZ == o->posZ
            && rotX == o->rotX && rotY == o->rotY && rotZ == o->rotZ && scale == o->scale
            && owner == o->owner && ownerExtra == o->ownerExtra && lockLevel == o->lockLevel
            && initiallyDisabled == o->initiallyDisabled && xespExtra == o->xespExtra
            && scriptIds == o->scriptIds
            && hasName == o->hasName && hasData == o->hasData
            && dataFloats == o->dataFloats;
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
        out.push_back(std::make_unique<UInt16EditorProperty>(
            QStringLiteral("Base Spell"), &baseSpell));
        out.push_back(std::make_unique<UInt16EditorProperty>(
            QStringLiteral("Fatigue"), &fatigue));
        out.push_back(std::make_unique<UInt16EditorProperty>(
            QStringLiteral("Barter Gold"), &barterGold));
        out.push_back(std::make_unique<Int16EditorProperty>(
            QStringLiteral("Level"), &level));
        out.push_back(std::make_unique<UInt16EditorProperty>(
            QStringLiteral("Calc Min"), &calcMin));
        out.push_back(std::make_unique<UInt16EditorProperty>(
            QStringLiteral("Calc Max"), &calcMax));
        out.push_back(std::make_unique<UInt16EditorProperty>(
            QStringLiteral("Speed Mult"), &speedMult));
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

    bool writeSubrecord(NAME subrecordName, ESMWriter& esm) const override
    {
        if (subrecordName == NAME('BODT'))
        {
            esm.startSubRecord(NAME('BODT'));
            esm.writeType<quint32>(partType);
            esm.writeType<quint32>(flags);
            esm.endSubRecord();
            return true;
        }
        if (subrecordName == NAME('BOD2'))
        {
            esm.startSubRecord(NAME('BOD2'));
            esm.writeType<quint32>(partType);
            esm.writeType<quint32>(flags);
            esm.writeType<quint32>(partCount);
            esm.endSubRecord();
            return true;
        }
        return false;
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
        out.push_back(std::make_unique<UInt8EnumEditorProperty>(QStringLiteral("Aggression"), &aggression,
            std::vector<UInt8EnumEditorProperty::Entry>{
                {"Unaggressive", 0}, {"Aggressive", 1}, {"Very Aggressive", 2},
                {"Frenzied", 3}, {"Defensive", 4}, {"Cowardly", 5}}));
        out.push_back(std::make_unique<UInt8EnumEditorProperty>(QStringLiteral("Confidence"), &confidence,
            std::vector<UInt8EnumEditorProperty::Entry>{
                {"Cowardly", 0}, {"Cautious", 1}, {"Average", 2},
                {"Brave", 3}, {"Foolhardy", 4}, {"Berserk", 5}}));
        out.push_back(std::make_unique<UInt8EnumEditorProperty>(QStringLiteral("Morality"), &morality,
            std::vector<UInt8EnumEditorProperty::Entry>{
                {"Any", 0}, {"Low", 1}, {"Standard", 2},
                {"High", 3}, {"None", 4}}));
        out.push_back(std::make_unique<UInt8EditorProperty>(QStringLiteral("Energy"), &energy));
        out.push_back(std::make_unique<Int16EditorProperty>(QStringLiteral("Mood"), &mood));
        out.push_back(std::make_unique<UInt8EditorProperty>(QStringLiteral("Mood Speed"), &moodSpeed));
        out.push_back(std::make_unique<UInt8EditorProperty>(QStringLiteral("Disposition"), &disposition));
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
    QVector<quint32> loadedIds;
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
            if (!loadedIds.contains(skillId))
                loadedIds.append(skillId);
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (loadedIds.isEmpty())
        {
            for (int i = 0; i < skillValues.size(); ++i)
            {
                if (skillValues[i] == 0) continue;
                esm.startSubRecord(NAME('SKIL'));
                esm.writeType<quint32>(static_cast<quint32>(i));
                esm.writeType<qint32>(skillValues[i]);
                esm.endSubRecord();
            }
            return;
        }
        for (quint32 id : loadedIds)
        {
            if (id >= static_cast<quint32>(skillValues.size()))
                continue;
            esm.startSubRecord(NAME('SKIL'));
            esm.writeType<quint32>(id);
            esm.writeType<qint32>(skillValues[id]);
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
        c->loadedIds = loadedIds;
        return c;
    }
    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const TESSkills_Component*>(other);
        skillValues = o->skillValues;
        loadedIds = o->loadedIds;
    }
    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const TESSkills_Component*>(other);
        return skillValues == o->skillValues && loadedIds == o->loadedIds;
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
    QVector<quint32> loadedIds;
    QVector<RawSubRecord> rawSub;
    quint32 spelling = NAME('ATTR');

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
            spelling = NAME('ATTR');
            qint64 count = esm.subLeft() / 2;
            attributes.clear();
            loadedIds.clear();
            attributes.reserve(count);
            for (qint64 i = 0; i < count; ++i)
            {
                attributes.append(esm.readType<qint16>());
                loadedIds.append(static_cast<quint32>(i));
            }
        }
        else if (subrecordName == NAME('BYDT'))
        {
            spelling = NAME('BYDT');
            quint8 attrId = esm.readType<quint8>();
            qint32 val = esm.readType<qint32>();
            if (attrId >= static_cast<quint32>(attributes.size()))
                attributes.resize(attrId + 1);
            attributes[attrId] = val;
            if (!loadedIds.contains(attrId))
                loadedIds.append(attrId);
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (attributes.isEmpty()) return;
        if (spelling == NAME('BYDT'))
        {
            const QVector<quint32> ids = loadedIds.isEmpty() ? [&] {
                QVector<quint32> all;
                for (int i = 0; i < attributes.size(); ++i)
                    all.append(static_cast<quint32>(i));
                return all;
            }() : loadedIds;
            for (quint32 id : ids)
            {
                if (id >= static_cast<quint32>(attributes.size()))
                    continue;
                esm.startSubRecord(NAME('BYDT'));
                esm.writeType<quint8>(static_cast<quint8>(id));
                esm.writeType<qint32>(attributes[id]);
                esm.endSubRecord();
            }
            return;
        }
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
        c->loadedIds = loadedIds;
        c->spelling = spelling;
        return c;
    }
    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const TESAttributes_Component*>(other);
        attributes = o->attributes;
        loadedIds = o->loadedIds;
        spelling = o->spelling;
    }
    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const TESAttributes_Component*>(other);
        return attributes == o->attributes
            && loadedIds == o->loadedIds
            && spelling == o->spelling;
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
            esm.writeRawSubRecord(raw);
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
