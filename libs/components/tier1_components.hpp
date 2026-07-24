#ifndef TIER1_COMPONENTS_HPP
#define TIER1_COMPONENTS_HPP

// =============================================================================
// Tier 1 Components (universal — used by every record type)
// =============================================================================
//
// Each component owns a slice of the record's subrecords. The
// record-level loader walks subrecords in order and dispatches each
// one to the first component that returns true from canHandle().
//
// Mapping to the real CK's classes is documented in
// docs/CK_Real_Integration_Plan.md. The names below match the CK's
// class names so debugging against real CK output is straightforward,
// but the implementations are ours.

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
// TESModel_Component — model + LOD model. Starfield uses MODL/MOD2
// (legacy) or MODT/MOD3 (Starfield-specific) depending on the form.
// We accept both.
// ---------------------------------------------------------------------------
class TESModel_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    QString modelPath;
    QString lodModelPath;

    QString name() const override { return QStringLiteral("Model"); }
    QString className() const override { return QStringLiteral("TESModel"); }
    static QString staticClassName() { return QStringLiteral("TESModel"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('MODL')
            || subrecordName == NAME('ODIT')
            || subrecordName == NAME('MNAM');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        if (subrecordName == NAME('MODL') || subrecordName == NAME('ODIT'))
        {
            modelPath = esm.readZString();
        }
        else if (subrecordName == NAME('MNAM'))
        {
            lodModelPath = esm.readZString();
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (!modelPath.isEmpty())
        {
            esm.writeSubZString(NAME('MODL'), modelPath);
        }
        if (!lodModelPath.isEmpty())
        {
            esm.writeSubZString(NAME('MNAM'), lodModelPath);
        }
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<StringEditorProperty>(
            QStringLiteral("Model File Name"), &modelPath));
        out.push_back(std::make_unique<StringEditorProperty>(
            QStringLiteral("LOD File Name"), &lodModelPath));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<TESModel_Component>();
        c->modelPath = modelPath;
        c->lodModelPath = lodModelPath;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const TESModel_Component*>(other);
        modelPath = o->modelPath;
        lodModelPath = o->lodModelPath;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const TESModel_Component*>(other);
        return modelPath == o->modelPath && lodModelPath == o->lodModelPath;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

// ---------------------------------------------------------------------------
// TESTexture_Component — inventory icon. Starfield uses ICON (legacy)
// or ITM2 (OpenCK's chosen tag) and ICO2 (small icon).
// ---------------------------------------------------------------------------
class TESTexture_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    QString iconPath;
    QString smallIconPath;

    QString name() const override { return QStringLiteral("Icon"); }
    QString className() const override { return QStringLiteral("TESTexture"); }
    static QString staticClassName() { return QStringLiteral("TESTexture"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('ICON')
            || subrecordName == NAME('ITM2')
            || subrecordName == NAME('ICO2');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        if (subrecordName == NAME('ICON') || subrecordName == NAME('ITM2'))
        {
            iconPath = esm.readZString();
        }
        else if (subrecordName == NAME('ICO2'))
        {
            smallIconPath = esm.readZString();
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (!iconPath.isEmpty())
        {
            esm.writeSubZString(NAME('ICON'), iconPath);
        }
        if (!smallIconPath.isEmpty())
        {
            esm.writeSubZString(NAME('ICO2'), smallIconPath);
        }
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<StringEditorProperty>(
            QStringLiteral("Inventory Icon"), &iconPath));
        out.push_back(std::make_unique<StringEditorProperty>(
            QStringLiteral("Small Inventory Icon"), &smallIconPath));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<TESTexture_Component>();
        c->iconPath = iconPath;
        c->smallIconPath = smallIconPath;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const TESTexture_Component*>(other);
        iconPath = o->iconPath;
        smallIconPath = o->smallIconPath;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const TESTexture_Component*>(other);
        return iconPath == o->iconPath && smallIconPath == o->smallIconPath;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

// ---------------------------------------------------------------------------
// TESHealth_Component — health value. Type varies between games
// (float in Morrowind, int in later games).
// ---------------------------------------------------------------------------
class TESHealth_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    float health = 0.0f;

    QString name() const override { return QStringLiteral("Health"); }
    QString className() const override { return QStringLiteral("TESHealth"); }
    static QString staticClassName() { return QStringLiteral("TESHealth"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('DATA') // ambiguous; we test
                                              // by the owner record
               || subrecordName == NAME('HLTH');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        if (subrecordName == NAME('HLTH'))
        {
            health = esm.readType<float>();
        }
        else if (subrecordName == NAME('DATA'))
        {
            // Many records store a single float in DATA. If the
            // subrecord is 4 bytes and we're owned by an
            // armor/weapon/etc., treat it as health. The record
            // loader's dispatch order is what makes this safe:
            // TESHealth is asked *first* only for records where
            // the parent record type owns the DATA slot as
            // health.
            const qint64 subSize = esm.subLeft();
            if (subSize == 4)
            {
                health = esm.readType<float>();
            }
            else if (subSize == 2)
            {
                quint16 v = esm.readType<quint16>();
                health = static_cast<float>(v);
            }
        }
    }

    void save(ESMWriter& esm) const override
    {
        esm.writeSubData<float>(NAME('HLTH'), health);
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<FloatEditorProperty>(
            QStringLiteral("Health"), &health));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<TESHealth_Component>();
        c->health = health;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const TESHealth_Component*>(other);
        health = o->health;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const TESHealth_Component*>(other);
        return health == o->health;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

// ---------------------------------------------------------------------------
// TESValue_Component — gold value (DATA subrecord, integer).
// ---------------------------------------------------------------------------
class TESValue_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    qint32 value = 0;

    QString name() const override { return QStringLiteral("Value"); }
    QString className() const override { return QStringLiteral("TESValue"); }
    static QString staticClassName() { return QStringLiteral("TESValue"); }

    // We don't claim DATA here; the record loader dispatches to us
    // via the explicit (component, subrecord) pair it built for
    // each record type. See load-by-dispatch in tescomponents.cpp.
    bool canHandle(quint32) const override { return false; }
    void handleSubrecord(quint32, ESMReader&) override {}

    void save(ESMWriter& esm) const override
    {
        esm.writeSubData<qint32>(NAME('DATA'), value);
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<IntEditorProperty>(
            QStringLiteral("Value"), &value));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<TESValue_Component>();
        c->value = value;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        value = static_cast<const TESValue_Component*>(other)->value;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        return value == static_cast<const TESValue_Component*>(other)->value;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

// ---------------------------------------------------------------------------
// TESWeight_Component — weight (DATA subrecord, float).
// ---------------------------------------------------------------------------
class TESWeight_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    float weight = 0.0f;

    QString name() const override { return QStringLiteral("Weight"); }
    QString className() const override { return QStringLiteral("TESWeight"); }
    static QString staticClassName() { return QStringLiteral("TESWeight"); }

    bool canHandle(quint32) const override { return false; }
    void handleSubrecord(quint32, ESMReader&) override {}

    void save(ESMWriter& esm) const override
    {
        esm.writeSubData<float>(NAME('DATA'), weight);
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<FloatEditorProperty>(
            QStringLiteral("Weight"), &weight));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<TESWeight_Component>();
        c->weight = weight;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        weight = static_cast<const TESWeight_Component*>(other)->weight;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        return weight == static_cast<const TESWeight_Component*>(other)->weight;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

// ---------------------------------------------------------------------------
// TESDescription_Component — long-form description text (DESC).
// ---------------------------------------------------------------------------
class TESDescription_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    QString description;

    QString name() const override { return QStringLiteral("Description"); }
    QString className() const override { return QStringLiteral("TESDescription"); }
    static QString staticClassName() { return QStringLiteral("TESDescription"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('DESC');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        if (subrecordName == NAME('DESC'))
        {
            description = esm.readZString();
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (!description.isEmpty())
        {
            esm.writeSubZString(NAME('DESC'), description);
        }
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<StringEditorProperty>(
            QStringLiteral("Description"), &description));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<TESDescription_Component>();
        c->description = description;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        description = static_cast<const TESDescription_Component*>(other)->description;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        return description == static_cast<const TESDescription_Component*>(other)->description;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

// ---------------------------------------------------------------------------
// BGSKeywordForm_Component — list of form-keywords attached to a
// record. The Starfield/CK form is a single CNAM subrecord
// containing a FormID list; older games use a per-keyword
// structure.
// ---------------------------------------------------------------------------
class BGSKeywordForm_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    QVector<quint32> keywords;

    QString name() const override { return QStringLiteral("Keywords"); }
    QString className() const override { return QStringLiteral("BGSKeywordForm"); }
    static QString staticClassName() { return QStringLiteral("BGSKeywordForm"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('CNAM')
            || subrecordName == NAME('KWDA');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        if (subrecordName == NAME('CNAM') || subrecordName == NAME('KWDA'))
        {
            const qint64 n = esm.subLeft() / 4;
            keywords.clear();
            keywords.reserve(n);
            for (qint64 i = 0; i < n; ++i)
            {
                keywords.append(esm.readType<quint32>());
            }
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (keywords.isEmpty()) return;
        esm.startSubRecord(NAME('CNAM'));
        for (quint32 kw : keywords)
        {
            esm.writeType<quint32>(kw);
        }
        esm.endSubRecord();
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<FormArrayEditorProperty>(
            QStringLiteral("Keywords"), &keywords));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<BGSKeywordForm_Component>();
        c->keywords = keywords;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        keywords = static_cast<const BGSKeywordForm_Component*>(other)->keywords;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        return keywords == static_cast<const BGSKeywordForm_Component*>(other)->keywords;
    }

    void mergeWith(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto& o = static_cast<const BGSKeywordForm_Component*>(other)->keywords;
        for (quint32 k : o)
        {
            if (!keywords.contains(k)) keywords.append(k);
        }
    }
};

} // namespace tescomponents

#endif // TIER1_COMPONENTS_HPP
