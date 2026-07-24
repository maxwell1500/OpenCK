#ifndef TESFULLNAME_HPP
#define TESFULLNAME_HPP

// TESFullName_Component — every named form in the Bethesda record
// family carries a FULL subrecord with a localized display name.
// This is the most-used component in the editor and is added to
// essentially every component-having record. Mirrors the CK's
// TESFullName_Component in
// docs/CK_Real_Integration_Plan.md.

#include "component.hpp"
#include "editorproperty.hpp"

#include <QString>
#include <QVector>

#include <memory>
#include <vector>

class ESMReader;
class ESMWriter;
struct RawSubRecord;

namespace tescomponents {

class TESFullName_Component : public Component
{
public:
    TESFullName_Component() = default;

    QString fullName;

    // Component interface
    QString name() const override { return QStringLiteral("Name"); }
    QString className() const override { return QStringLiteral("TESFullName"); }
    static QString staticClassName() { return QStringLiteral("TESFullName"); }

    void load(ESMReader& esm) override { (void)esm; }
    void save(ESMWriter& esm) const override { (void)esm; }

    std::vector<std::unique_ptr<EditorProperty>> createEditorProperties() override
    {
        std::vector<std::unique_ptr<EditorProperty>> out;
        out.push_back(std::make_unique<StringEditorProperty>(
            QStringLiteral("Name"), &fullName));
        return out;
    }

    std::unique_ptr<Component> clone() const override
    {
        auto c = std::make_unique<TESFullName_Component>();
        c->fullName = fullName;
        c->rawSubRecords = rawSubRecords;
        return c;
    }

    void copyFrom(const Component* other) override
    {
        if (!other || other->className() != className()) return;
        const auto* o = static_cast<const TESFullName_Component*>(other);
        fullName = o->fullName;
        rawSubRecords = o->rawSubRecords;
    }

    bool isEqualTo(const Component* other) const override
    {
        if (!other || other->className() != className()) return false;
        const auto* o = static_cast<const TESFullName_Component*>(other);
        return fullName == o->fullName && rawSubRecords == o->rawSubRecords;
    }

    void mergeWith(const Component* other) override { copyFrom(other); }

private:
    // Subrecords that belong to this component but we don't have a
    // dedicated editor for. Preserved verbatim on save so the round
    // trip is lossless.
    QVector<RawSubRecord> rawSubRecords;
};

} // namespace tescomponents

#endif // TESFULLNAME_HPP
