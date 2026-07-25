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
class TESFlags_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    quint32 flags = 0;

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
        out.push_back(std::make_unique<UIntEditorProperty>(
            QStringLiteral("Flags"), &flags));
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
// Handles SNAM (sun texture path) and FNAM/FLAG (flags).
// ---------------------------------------------------------------------------
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
        esm.writeType<float>(scale);
        esm.endSubRecord();
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

} // namespace tescomponents

#endif // TIER3_COMPONENTS_HPP
