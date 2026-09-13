#ifndef TES3_COMPONENTS_HPP
#define TES3_COMPONENTS_HPP

// TES3 (Morrowind) specific components. These handle TES3 record
// subrecords that differ from the TES4 format.

#include "component.hpp"
#include "editorproperty.hpp"

#include <QString>
#include <QByteArray>

#include <memory>
#include <vector>

class ESMReader;
class ESMWriter;

namespace tescomponents {

/// Generic binary DATA subrecord handler for TES3 records. Captures the raw
/// bytes of the DATA subrecord and exposes it as hex for inspection/editing.
/// Type-specific parsing of DATA payloads is deferred to a later phase.
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
        return true;
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

} // namespace tescomponents

#endif // TES3_COMPONENTS_HPP
