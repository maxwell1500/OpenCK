#ifndef TIER2_COMPONENTS_HPP
#define TIER2_COMPONENTS_HPP

// =============================================================================
// Tier 2 Components (equipment-specific)
// =============================================================================
//
// Mirrors the real CK's component class family. See
// docs/CK_Real_Integration_Plan.md for the cross-reference.

#include "component.hpp"
#include "editorproperty.hpp"
#include "../files/esm/esmreader.hpp"

#include <QString>
#include <QVector>

#include <cstring>
#include <memory>
#include <vector>

class ESMReader;
class ESMWriter;
struct RawSubRecord;

namespace tescomponents {

// ---------------------------------------------------------------------------
// TESBipedModel_Component — armor/clothing biped slots + per-gender
// models. Morrowind/Skyrim use BNAM + INDX/INDT/CNAM/FNAM/MNAM
// subrecords; OpenCK's existing record format collapses the
// per-gender split into maleWorldPath/femaleWorldPath for simplicity.
// ---------------------------------------------------------------------------
/// Armor/clothing biped slots and per-gender world model paths.
class TESBipedModel_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    QString maleWorldPath;
    QString femaleWorldPath;
    quint32 bipedFlags = 0;
    quint32 maleSpelling = NAME('BNAM');
    quint32 flagsSpelling = NAME('BMDT');
    bool hasMale = false;
    bool hasFemale = false;
    bool hasFlags = false;
    // Verbatim snapshots for positional replay: unedited values re-emit the
    // exact source bytes (spelling, width, NULs); edits emit current values.
    QByteArray maleRaw;
    QByteArray femaleRaw;
    NAME maleRawName = 0;
    NAME femaleRawName = 0;
    QString loadedMale;
    QString loadedFemale;
    quint8 bipedWidth = 4;
    QByteArray bipedExtra;
    // Per-occurrence INDX/BMDT payloads (duplicates replay verbatim except
    // the last, which goes through writeSubrecord so edits land there).
    QVector<QByteArray> indxRaws;
    QVector<QByteArray> bmdtRaws;
    QVector<RawSubRecord> rawSub;

    QString name() const override { return QStringLiteral("Biped Model"); }
    QString className() const override { return QStringLiteral("TESBipedModel"); }
    static QString staticClassName() { return QStringLiteral("TESBipedModel"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('BNAM')
            || subrecordName == NAME('CNAM')
            || subrecordName == NAME('FNAM')
            || subrecordName == NAME('INDX')
            || subrecordName == NAME('INDT')
            || subrecordName == NAME('BMDT');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        switch (subrecordName)
        {
            case NAME('BNAM'):
            case NAME('CNAM'):
            {
                maleSpelling = subrecordName;
                maleRawName = subrecordName;
                hasMale = true;
                esm.readRawSubData(maleRaw);
                maleWorldPath = QString::fromUtf8(maleRaw.constData(), maleRaw.size());
                while (maleWorldPath.endsWith(QChar(0)))
                    maleWorldPath.chop(1);
                loadedMale = maleWorldPath;
                break;
            }
            case NAME('FNAM'):
            {
                hasFemale = true;
                femaleRawName = subrecordName;
                esm.readRawSubData(femaleRaw);
                femaleWorldPath = QString::fromUtf8(femaleRaw.constData(), femaleRaw.size());
                while (femaleWorldPath.endsWith(QChar(0)))
                    femaleWorldPath.chop(1);
                loadedFemale = femaleWorldPath;
                break;
            }
            case NAME('INDX'):
            case NAME('BMDT'):
            {
                // Width-preserving scalar: Starfield writes 2-byte INDX
                // here; a fixed u32 read would consume the next subrecord.
                flagsSpelling = subrecordName;
                const qint64 left = esm.subLeft();
                bipedExtra.clear();
                QByteArray payload;
                if (left >= 4)
                {
                    bipedFlags = esm.readType<quint32>();
                    bipedWidth = 4;
                    for (int i = 0; i < 4; ++i)
                        payload.append(char((bipedFlags >> (8 * i)) & 0xFF));
                }
                else
                {
                    bipedFlags = 0;
                    bipedWidth = static_cast<quint8>(qMax<qint64>(left, 0));
                    for (qint64 i = 0; i < left; ++i)
                    {
                        const quint8 byte = esm.readType<quint8>();
                        bipedFlags |= quint32(byte) << (8 * i);
                        payload.append(char(byte));
                    }
                }
                if (esm.subLeft() > 0)
                {
                    esm.readRawSubData(bipedExtra);
                    payload.append(bipedExtra);
                }
                if (subrecordName == NAME('INDX'))
                    indxRaws.append(payload);
                else
                    bmdtRaws.append(payload);
                hasFlags = true;
                break;
            }
            case NAME('INDT'):
            default:
                RawSubRecord raw;
                raw.name = subrecordName;
                esm.readRawSubData(raw.data);
                rawSub.push_back(raw);
                break;
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (hasFlags || bipedFlags != 0)
        {
            quint8 w = bipedWidth;
            if (w == 0 && bipedFlags != 0)
                w = 4;
            esm.startSubRecord(flagsSpelling);
            for (quint8 i = 0; i < w; ++i)
                esm.writeType<quint8>(static_cast<quint8>((bipedFlags >> (8 * i)) & 0xFF));
            if (!bipedExtra.isEmpty())
                esm.writeRawData(bipedExtra.constData(), bipedExtra.size());
            esm.endSubRecord();
        }
        if (hasMale || !maleWorldPath.isEmpty())
        {
            esm.writeSubZString(maleSpelling, maleWorldPath);
        }
        if (hasFemale || !femaleWorldPath.isEmpty())
        {
            esm.writeSubZString(NAME('FNAM'), femaleWorldPath);
        }
        for (const auto& raw : rawSub)
        {
            esm.writeRawSubRecord(raw);
        }
    }

    bool writeSubrecord(NAME subrecordName, ESMWriter& esm) const override
    {
        if (subrecordName == NAME('BNAM') || subrecordName == NAME('CNAM'))
        {
            if (!maleRaw.isEmpty() && maleWorldPath == loadedMale)
                esm.writeRawSubRecord(RawSubRecord{ maleRawName, maleRaw });
            else if (!maleWorldPath.isEmpty())
                esm.writeSubZString(subrecordName, maleWorldPath);
            else if (hasMale)
            {
                esm.startSubRecord(subrecordName);
                esm.endSubRecord();
            }
            return true;
        }
        if (subrecordName == NAME('FNAM'))
        {
            if (!femaleRaw.isEmpty() && femaleWorldPath == loadedFemale)
                esm.writeRawSubRecord(RawSubRecord{ femaleRawName, femaleRaw });
            else if (!femaleWorldPath.isEmpty())
                esm.writeSubZString(subrecordName, femaleWorldPath);
            else if (hasFemale)
            {
                esm.startSubRecord(subrecordName);
                esm.endSubRecord();
            }
            return true;
        }
        if (subrecordName == NAME('INDX') || subrecordName == NAME('BMDT'))
        {
            if (hasFlags || bipedFlags != 0)
            {
                quint8 w = bipedWidth;
                if (w == 0 && bipedFlags != 0)
                    w = 4;
                esm.startSubRecord(subrecordName);
                for (quint8 i = 0; i < w; ++i)
                    esm.writeType<quint8>(static_cast<quint8>((bipedFlags >> (8 * i)) & 0xFF));
                if (!bipedExtra.isEmpty())
                    esm.writeRawData(bipedExtra.constData(), bipedExtra.size());
                esm.endSubRecord();
            }
            return true;
        }
        return false;
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<UIntEditorProperty>(
            QStringLiteral("Biped Flags"), &bipedFlags));
        out.push_back(std::make_unique<StringEditorProperty>(
            QStringLiteral("Male World Model"), &maleWorldPath));
        out.push_back(std::make_unique<StringEditorProperty>(
            QStringLiteral("Female World Model"), &femaleWorldPath));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<TESBipedModel_Component>();
        c->maleWorldPath = maleWorldPath;
        c->femaleWorldPath = femaleWorldPath;
        c->bipedFlags = bipedFlags;
        c->maleSpelling = maleSpelling;
        c->flagsSpelling = flagsSpelling;
        c->hasMale = hasMale;
        c->hasFemale = hasFemale;
        c->hasFlags = hasFlags;
        c->maleRaw = maleRaw;
        c->femaleRaw = femaleRaw;
        c->maleRawName = maleRawName;
        c->femaleRawName = femaleRawName;
        c->loadedMale = loadedMale;
        c->loadedFemale = loadedFemale;
        c->bipedWidth = bipedWidth;
        c->bipedExtra = bipedExtra;
        c->indxRaws = indxRaws;
        c->bmdtRaws = bmdtRaws;
        c->rawSub = rawSub;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const TESBipedModel_Component*>(other);
        maleWorldPath = o->maleWorldPath;
        femaleWorldPath = o->femaleWorldPath;
        bipedFlags = o->bipedFlags;
        maleSpelling = o->maleSpelling;
        flagsSpelling = o->flagsSpelling;
        hasMale = o->hasMale;
        hasFemale = o->hasFemale;
        hasFlags = o->hasFlags;
        maleRaw = o->maleRaw;
        femaleRaw = o->femaleRaw;
        maleRawName = o->maleRawName;
        femaleRawName = o->femaleRawName;
        loadedMale = o->loadedMale;
        loadedFemale = o->loadedFemale;
        bipedWidth = o->bipedWidth;
        bipedExtra = o->bipedExtra;
        indxRaws = o->indxRaws;
        bmdtRaws = o->bmdtRaws;
        rawSub = o->rawSub;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const TESBipedModel_Component*>(other);
        return maleWorldPath == o->maleWorldPath
            && femaleWorldPath == o->femaleWorldPath
            && bipedFlags == o->bipedFlags
            && maleSpelling == o->maleSpelling
            && flagsSpelling == o->flagsSpelling
            && hasMale == o->hasMale
            && hasFemale == o->hasFemale
            && hasFlags == o->hasFlags
            && maleRaw == o->maleRaw
            && femaleRaw == o->femaleRaw
            && maleRawName == o->maleRawName
            && femaleRawName == o->femaleRawName
            && loadedMale == o->loadedMale
            && loadedFemale == o->loadedFemale
            && bipedWidth == o->bipedWidth
            && bipedExtra == o->bipedExtra
            && indxRaws == o->indxRaws
            && bmdtRaws == o->bmdtRaws
            && rawSub == o->rawSub;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

// ---------------------------------------------------------------------------
// TESEnchantableForm_Component — attached enchantment + max charge.
// ---------------------------------------------------------------------------
/// Attached enchantment form ID and maximum charge.
class TESEnchantableForm_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    quint32 enchantmentFormId = 0;
    quint32 maxCharge = 0;
    quint32 spelling = NAME('ENAM');
    bool hasEnchant = false;

    QString name() const override { return QStringLiteral("Enchantment"); }
    QString className() const override { return QStringLiteral("TESEnchantableForm"); }
    static QString staticClassName() { return QStringLiteral("TESEnchantableForm"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('ENAM')
            || subrecordName == NAME('ANAM');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        if (subrecordName == NAME('ENAM') || subrecordName == NAME('ANAM'))
        {
            spelling = subrecordName;
            enchantmentFormId = esm.readType<quint32>();
            hasEnchant = true;
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (hasEnchant || enchantmentFormId != 0)
        {
            esm.writeSubData<quint32>(spelling, enchantmentFormId);
        }
    }

    bool writeSubrecord(NAME subrecordName, ESMWriter& esm) const override
    {
        if (subrecordName != NAME('ENAM') && subrecordName != NAME('ANAM'))
            return false;
        if (hasEnchant || enchantmentFormId != 0)
        {
            esm.writeSubData<quint32>(subrecordName, enchantmentFormId);
        }
        return true;
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<FormEditorProperty>(
            QStringLiteral("Enchantment"), &enchantmentFormId));
        out.push_back(std::make_unique<UIntEditorProperty>(
            QStringLiteral("Max Charge"), &maxCharge));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<TESEnchantableForm_Component>();
        c->enchantmentFormId = enchantmentFormId;
        c->maxCharge = maxCharge;
        c->spelling = spelling;
        c->hasEnchant = hasEnchant;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const TESEnchantableForm_Component*>(other);
        enchantmentFormId = o->enchantmentFormId;
        maxCharge = o->maxCharge;
        spelling = o->spelling;
        hasEnchant = o->hasEnchant;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const TESEnchantableForm_Component*>(other);
        return enchantmentFormId == o->enchantmentFormId
            && maxCharge == o->maxCharge
            && spelling == o->spelling
            && hasEnchant == o->hasEnchant;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

// ---------------------------------------------------------------------------
// BGSPickupPutdownSounds_Component — sound forms for inventory
// pickup/putdown. Starfield uses YNAM/ZNAM (legacy) or the
// starfield-specific PICK/PUT subrecords.
// ---------------------------------------------------------------------------
/// Pickup and putdown sound form IDs (YNAM/ZNAM/PICK/PUTD subrecords).
class BGSPickupPutdownSounds_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    quint32 pickupSound = 0;
    quint32 putdownSound = 0;
    quint32 pickupSpelling = NAME('YNAM');
    quint32 putdownSpelling = NAME('ZNAM');
    // Every occurrence payload (YNAM/ZNAM repeat on ALCH/ARMO etc.); the
    // scalar fields above track the last.
    QVector<QByteArray> ynamRaws;
    QVector<QByteArray> znamRaws;

    QString name() const override { return QStringLiteral("Pickup / Putdown Sounds"); }
    QString className() const override { return QStringLiteral("BGSPickupPutdownSounds"); }
    static QString staticClassName() { return QStringLiteral("BGSPickupPutdownSounds"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('YNAM')
            || subrecordName == NAME('ZNAM')
            || subrecordName == NAME('PICK')
            || subrecordName == NAME('PUTD');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        if (subrecordName == NAME('YNAM') || subrecordName == NAME('PICK'))
        {
            pickupSpelling = subrecordName;
            QByteArray raw;
            esm.readRawSubData(raw);
            ynamRaws.append(raw);
            quint32 v = 0;
            if (raw.size() >= 4) memcpy(&v, raw.constData(), 4);
            pickupSound = v;
        }
        else if (subrecordName == NAME('ZNAM') || subrecordName == NAME('PUTD'))
        {
            putdownSpelling = subrecordName;
            QByteArray raw;
            esm.readRawSubData(raw);
            znamRaws.append(raw);
            quint32 v = 0;
            if (raw.size() >= 4) memcpy(&v, raw.constData(), 4);
            putdownSound = v;
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (pickupSound != 0)
        {
            esm.writeSubData<quint32>(pickupSpelling, pickupSound);
        }
        if (putdownSound != 0)
        {
            esm.writeSubData<quint32>(putdownSpelling, putdownSound);
        }
    }

    bool writeSubrecord(NAME subrecordName, ESMWriter& esm) const override
    {
        if (subrecordName == NAME('YNAM') || subrecordName == NAME('PICK'))
        {
            if (pickupSound != 0) esm.writeSubData<quint32>(subrecordName, pickupSound);
            return true;
        }
        if (subrecordName == NAME('ZNAM') || subrecordName == NAME('PUTD'))
        {
            if (putdownSound != 0) esm.writeSubData<quint32>(subrecordName, putdownSound);
            return true;
        }
        return false;
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<FormEditorProperty>(
            QStringLiteral("Pickup Sound"), &pickupSound));
        out.push_back(std::make_unique<FormEditorProperty>(
            QStringLiteral("Putdown Sound"), &putdownSound));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<BGSPickupPutdownSounds_Component>();
        c->pickupSound = pickupSound;
        c->putdownSound = putdownSound;
        c->pickupSpelling = pickupSpelling;
        c->putdownSpelling = putdownSpelling;
        c->ynamRaws = ynamRaws;
        c->znamRaws = znamRaws;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const BGSPickupPutdownSounds_Component*>(other);
        pickupSound = o->pickupSound;
        putdownSound = o->putdownSound;
        pickupSpelling = o->pickupSpelling;
        putdownSpelling = o->putdownSpelling;
        ynamRaws = o->ynamRaws;
        znamRaws = o->znamRaws;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const BGSPickupPutdownSounds_Component*>(other);
        return pickupSound == o->pickupSound && putdownSound == o->putdownSound
            && pickupSpelling == o->pickupSpelling
            && putdownSpelling == o->putdownSpelling
            && ynamRaws == o->ynamRaws && znamRaws == o->znamRaws;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

// ---------------------------------------------------------------------------
// BGSInstanceNamingRulesForm_Component — Starfield's rules for how
// the engine generates a display name for instanced forms
// (e.g. an apple from an apple crate is "Apple" with no Editor ID).
// Stored as a small set of subrecords; we just preserve the bytes.
// ---------------------------------------------------------------------------
/// Starfield instance naming rules, preserved as raw subrecord bytes.
class BGSInstanceNamingRulesForm_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    QVector<RawSubRecord> rawSubRecords;

    QString name() const override { return QStringLiteral("Instance Naming Rules"); }
    QString className() const override { return QStringLiteral("BGSInstanceNamingRulesForm"); }
    static QString staticClassName() { return QStringLiteral("BGSInstanceNamingRulesForm"); }

    bool canHandle(quint32 subrecordName) const override
    {
        // We claim the INRR + INRV + INRD subrecord family used by
        // Starfield instance naming. Real parsing of these into
        // structured rules is a Phase E follow-up (see
        // docs/REMAINING_WORK_PLAN.md); for now we preserve the raw
        // bytes.
        return subrecordName == NAME('INRR')
            || subrecordName == NAME('INRV')
            || subrecordName == NAME('INRD');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        Q_UNUSED(subrecordName);
        RawSubRecord raw;
        raw.name = subrecordName;
        esm.readRawSubData(raw.data);
        rawSubRecords.push_back(raw);
    }

    void save(ESMWriter& esm) const override
    {
        for (const auto& raw : rawSubRecords)
        {
            esm.writeRawSubRecord(raw);
        }
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        // No editable properties yet — the underlying data isn't
        // structured. Tier 3 enhancement.
        return {};
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<BGSInstanceNamingRulesForm_Component>();
        c->rawSubRecords = rawSubRecords;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        rawSubRecords = static_cast<const BGSInstanceNamingRulesForm_Component*>(other)->rawSubRecords;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        return rawSubRecords == static_cast<const BGSInstanceNamingRulesForm_Component*>(other)->rawSubRecords;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

} // namespace tescomponents

#endif // TIER2_COMPONENTS_HPP
