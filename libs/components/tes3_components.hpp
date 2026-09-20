#ifndef TES3_COMPONENTS_HPP
#define TES3_COMPONENTS_HPP

// TES3 (Morrowind) specific components. These handle TES3 record
// subrecords that differ from the TES4 format.

#include "component.hpp"
#include "editorproperty.hpp"
#include "../files/esm/tes3datalayout.hpp"

#include <QString>
#include <QByteArray>
#include <QVector>
#include <QVariant>

#include <memory>
#include <vector>

class ESMReader;
class ESMWriter;

namespace tescomponents {

/// Generic binary DATA subrecord handler for TES3 records. Captures the raw
/// bytes of the DATA subrecord and exposes it as hex for inspection/editing.
/// When the owning record's (code, size) has a proven typed layout
/// (tes3datalayout.*), the payload is additionally decoded into typed fields
/// with per-field editor properties; field edits re-encode into the raw
/// bytes, so save/undo/hex all follow. Unknown layouts stay hex-only.
class Tes3Data_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    QByteArray data;
    NAME nameSpelling = NAME('DATA');
    NAME recordCode = 0;     // owning record code (set by Tes3Record::parseComponents)
    int dataOccurrence = -1; // which DATA occurrence data mirrors (-1 = none)
    bool stringNullTerminated = false;

    struct TypedField
    {
        QString name;
        Tes3DataFieldType type = Tes3DataFieldType::U8;
        QVariant value;
        QString tooltip;
    };
    QVector<TypedField> typedFields;
    bool typedValid = false;

    void decode(NAME code, const QByteArray& payload)
    {
        recordCode = code;
        typedFields.clear();
        typedValid = false;
        stringNullTerminated = !payload.isEmpty() && payload.endsWith('\0');
        const Tes3DataLayout* layout = tes3DataLayoutFor(code, payload.size());
        if (!layout)
            return;
        for (int i = 0; i < layout->fieldCount; ++i)
        {
            const Tes3DataFieldDef& def = layout->fields[i];
            const QVariant v = tes3DataDecodeField(def, payload);
            if (!v.isValid())
                return;
            TypedField f;
            f.name = QString::fromUtf8(def.name);
            f.type = def.type;
            f.value = v;
            f.tooltip = QString::fromUtf8(def.tooltip);
            typedFields.append(f);
        }
        typedValid = true;
    }

    // Updates one typed field and re-encodes the raw payload. Unsigned
    // widths are clamped (matching IntEditorProperty); anything the layout
    // cannot encode is rejected with false and leaves the payload intact.
    bool setTypedValue(int index, const QVariant& v)
    {
        if (!typedValid || index < 0 || index >= typedFields.size())
            return false;
        const Tes3DataLayout* layout = tes3DataLayoutFor(recordCode, data.size());
        if (!layout || index >= layout->fieldCount)
            return false;
        const Tes3DataFieldDef& def = layout->fields[index];
        if (QString::fromUtf8(def.name) != typedFields[index].name)
            return false;
        if (def.type == Tes3DataFieldType::String)
        {
            QByteArray encoded = v.toString().toUtf8();
            if (stringNullTerminated)
                encoded.append('\0');
            data = encoded;
            typedFields[index].value = v.toString();
            return true;
        }
        bool ok = false;
        const quint64 uv = v.toULongLong(&ok);
        QVariant clamped = v;
        if (ok)
        {
            if (def.type == Tes3DataFieldType::U8)
                clamped = QVariant(static_cast<quint32>(qBound<quint64>(0, uv, 0xFFULL)));
            else if (def.type == Tes3DataFieldType::U16)
                clamped = QVariant(static_cast<quint32>(qBound<quint64>(0, uv, 0xFFFFULL)));
            else if (def.type == Tes3DataFieldType::U32)
                clamped = QVariant(static_cast<quint32>(qBound<quint64>(0, uv, 0xFFFFFFFFULL)));
        }
        QByteArray encoded = data;
        if (!tes3DataEncodeField(def, clamped, encoded))
            return false;
        data = encoded;
        typedFields[index].value = tes3DataDecodeField(def, data);
        return true;
    }

    QString name() const override { return QStringLiteral("Data"); }
    QString className() const override { return QStringLiteral("Tes3Data"); }
    static QString staticClassName() { return QStringLiteral("Tes3Data"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('DATA');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        if (subrecordName != NAME('DATA'))
            return;
        nameSpelling = NAME('DATA');
        data.clear();
        esm.readRawSubData(data);
    }

    void save(ESMWriter& esm) const override
    {
        if (data.isEmpty())
            return;
        esm.startSubRecord(nameSpelling);
        esm.writeRawData(data.constData(), data.size());
        esm.endSubRecord();
    }

    QString toHex() const
    {
        return QString(data.toHex().data()).toUpper();
    }

    bool fromHex(const QString& hex)
    {
        QString cleaned = hex;
        cleaned.remove(' ');
        QByteArray bytes = QByteArray::fromHex(cleaned.toUtf8());
        if (bytes.isEmpty() && !hex.isEmpty())
            return false;
        data = bytes;
        decode(recordCode, data);
        return true;
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override;

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<Tes3Data_Component>();
        c->data = data;
        c->nameSpelling = nameSpelling;
        c->recordCode = recordCode;
        c->dataOccurrence = dataOccurrence;
        c->stringNullTerminated = stringNullTerminated;
        c->typedFields = typedFields;
        c->typedValid = typedValid;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const Tes3Data_Component*>(other);
        data = o->data;
        nameSpelling = o->nameSpelling;
        recordCode = o->recordCode;
        dataOccurrence = o->dataOccurrence;
        stringNullTerminated = o->stringNullTerminated;
        typedFields = o->typedFields;
        typedValid = o->typedValid;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const Tes3Data_Component*>(other);
        return data == o->data;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

/// Editor property bound to one Tes3Data_Component typed field (or the whole
/// hex payload for index -1). Binding by (component, index) instead of raw
/// field pointers keeps the property valid across QVector reallocations.
class Tes3DataFieldProperty : public EditorProperty
{
public:
    Tes3DataFieldProperty(QString name, Tes3Data_Component* comp, int fieldIndex,
                          QString tooltip = QString())
        : m_name(std::move(name)), m_comp(comp), m_index(fieldIndex), m_tip(std::move(tooltip)) {}

    QString name() const override { return m_name; }
    QString toolTip() const override { return m_tip; }

    QVariant value() const override
    {
        if (!m_comp)
            return QVariant();
        if (m_index < 0)
            return m_comp->toHex();
        if (m_index >= m_comp->typedFields.size())
            return QVariant();
        return m_comp->typedFields[m_index].value;
    }

    void setValue(const QVariant& v) override
    {
        if (!m_comp)
            return;
        if (m_index < 0)
        {
            m_comp->fromHex(v.toString());
            return;
        }
        m_comp->setTypedValue(m_index, v);
    }

private:
    QString m_name;
    Tes3Data_Component* m_comp;
    int m_index;
    QString m_tip;
};

inline std::vector<std::unique_ptr<EditorProperty>> Tes3Data_Component::createEditorProperties()
{
    std::vector<std::unique_ptr<EditorProperty>> out;
    if (typedValid)
    {
        for (int i = 0; i < typedFields.size(); ++i)
        {
            out.push_back(std::make_unique<Tes3DataFieldProperty>(
                typedFields[i].name, this, i, typedFields[i].tooltip));
        }
    }
    out.push_back(std::make_unique<Tes3DataFieldProperty>(
        QStringLiteral("DATA (hex)"), this, -1,
        QStringLiteral("Raw payload view; edits re-decode the typed fields above.")));
    return out;
}

} // namespace tescomponents

#endif // TES3_COMPONENTS_HPP
