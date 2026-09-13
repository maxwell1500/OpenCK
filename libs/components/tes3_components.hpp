#ifndef TES3_COMPONENTS_HPP
#define TES3_COMPONENTS_HPP

// TES3 (Morrowind) specific components. These handle TES3 record
// subrecords that differ from the TES4 format.

#include "component.hpp"
#include "editorproperty.hpp"

#include <QString>
#include <QByteArray>
#include <QVector>

#include <memory>
#include <vector>

class ESMReader;
class ESMWriter;

namespace tescomponents {

/// Generic binary DATA subrecord handler for TES3 records. Exposes the raw
/// bytes as a hex string for inspection and editing. Type-specific parsing
/// of DATA payloads is deferred to a later phase.
class Tes3Data_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    QByteArray data;
    NAME nameSpelling = NAME('DATA');

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
        const qint64 left = esm.bytesLeftInRecord();
        data = QByteArray(left, 0);
        esm.readBytes(data.data(), left);
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
        return data.toHex(' ').toUpper();
    }

    bool fromHex(const QString& hex)
    {
        data = QByteArray::fromHex(hex.remove(' ').toUtf8());
        return !hex.isEmpty();
    }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<StringEditorProperty>(
            QStringLiteral("DATA (hex)"), &m_hexDisplay));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<Tes3Data_Component>();
        c->data = data;
        c->nameSpelling = nameSpelling;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const Tes3Data_Component*>(other);
        data = o->data;
        nameSpelling = o->nameSpelling;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const Tes3Data_Component*>(other);
        return data == o->data;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }

private:
    QString m_hexDisplay;
    friend class EditorProperty;
};

/// TES3 flags component — handles the DATA subrecord for records where
/// the first field is a flags word. Stores as a bitmask property.
class Tes3Flags_Component : public Component
{
public:
    void load(ESMReader& esm) override {}

    quint32 flags = 0;
    NAME nameSpelling = NAME('DATA');
    bool hasData = false;

    QString name() const override { return QStringLiteral("Flags"); }
    QString className() const override { return QStringLiteral("Tes3Flags"); }
    static QString staticClassName() { return QStringLiteral("Tes3Flags"); }

    bool canHandle(quint32 subrecordName) const override
    {
        return subrecordName == NAME('DATA');
    }

    void handleSubrecord(quint32 subrecordName, ESMReader& esm) override
    {
        if (subrecordName != NAME('DATA'))
            return;
        nameSpelling = NAME('DATA');
        hasData = true;
        flags = esm.readType<quint32>();
        // Drain any remaining bytes in the DATA subrecord
        const qint64 left = esm.bytesLeftInRecord();
        if (left > 0)
        {
            QByteArray drain(left, 0);
            esm.readBytes(drain.data(), left);
        }
    }

    void save(ESMWriter& esm) const override
    {
        if (!hasData)
            return;
        esm.writeSubData<quint32>(nameSpelling, flags);
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
        auto c = std::make_unique<Tes3Flags_Component>();
        c->flags = flags;
        c->hasData = hasData;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const Tes3Flags_Component*>(other);
        flags = o->flags;
        hasData = o->hasData;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const Tes3Flags_Component*>(other);
        return flags == o->flags;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }
};

} // namespace tescomponents

#endif // TES3_COMPONENTS_HPP
