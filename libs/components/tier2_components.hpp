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
                maleWorldPath = esm.readZString();
                break;
            case NAME('FNAM'):
                femaleWorldPath = esm.readZString();
                break;
            case NAME('INDX'):
                bipedFlags = esm.readType<quint32>();
                break;
            case NAME('BMDT'):
                bipedFlags = esm.readType<quint32>();
                break;
            default:
                break;
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (bipedFlags != 0)
        {
            esm.writeSubData<quint32>(NAME('BMDT'), bipedFlags);
        }
        if (!maleWorldPath.isEmpty())
        {
            esm.writeSubZString(NAME('BNAM'), maleWorldPath);
        }
        if (!femaleWorldPath.isEmpty())
        {
            esm.writeSubZString(NAME('FNAM'), femaleWorldPath);
        }
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
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const TESBipedModel_Component*>(other);
        maleWorldPath = o->maleWorldPath;
        femaleWorldPath = o->femaleWorldPath;
        bipedFlags = o->bipedFlags;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const TESBipedModel_Component*>(other);
        return maleWorldPath == o->maleWorldPath
            && femaleWorldPath == o->femaleWorldPath
            && bipedFlags == o->bipedFlags;
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
            enchantmentFormId = esm.readType<quint32>();
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (enchantmentFormId != 0)
        {
            esm.writeSubData<quint32>(NAME('ENAM'), enchantmentFormId);
        }
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
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const TESEnchantableForm_Component*>(other);
        enchantmentFormId = o->enchantmentFormId;
        maxCharge = o->maxCharge;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const TESEnchantableForm_Component*>(other);
        return enchantmentFormId == o->enchantmentFormId
            && maxCharge == o->maxCharge;
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
            pickupSound = esm.readType<quint32>();
        }
        else if (subrecordName == NAME('ZNAM') || subrecordName == NAME('PUTD'))
        {
            putdownSound = esm.readType<quint32>();
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (pickupSound != 0)
        {
            esm.writeSubData<quint32>(NAME('YNAM'), pickupSound);
        }
        if (putdownSound != 0)
        {
            esm.writeSubData<quint32>(NAME('ZNAM'), putdownSound);
        }
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
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const BGSPickupPutdownSounds_Component*>(other);
        pickupSound = o->pickupSound;
        putdownSound = o->putdownSound;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const BGSPickupPutdownSounds_Component*>(other);
        return pickupSound == o->pickupSound && putdownSound == o->putdownSound;
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
