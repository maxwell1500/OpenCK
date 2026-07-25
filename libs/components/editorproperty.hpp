#ifndef EDITOR_PROPERTY_HPP
#define EDITOR_PROPERTY_HPP

// =============================================================================
// EditorProperty — leaf-level editor nodes that the property grid renders.
//
// Each component owns a list of these and exposes them via
// Component::createEditorProperties(). The property grid walks the
// list, instantiates one editor widget per property, and binds it
// to the data via getValue() / setValue().
//
// Types map roughly to the CK's BGS*EditorProperty family (see
// docs/CK_Real_Integration_Plan.md for the cross-reference):
//
//   BoolEditorProperty           ~ BGSBoolEditorProperty
//   IntEditorProperty            ~ BGSInt32EditorProperty (and the
//                                   8/16/32/64-bit variants the CK
//                                   uses for size-typed integers)
//   FloatEditorProperty          ~ BGSFloatEditorProperty
//   StringEditorProperty         ~ BGSStringEditorProperty
//   FormEditorProperty           ~ BGSFormEditorProperty (form-ID picker)
//   FormArrayEditorProperty      ~ BGSFormArrayEditorProperty
//   FormComponentArrayEditorProperty ~ BGSFormComponentArrayEditorProperty
//                                   (array of nested components, e.g.
//                                    container items)
//   EnumEditorProperty           ~ BGSEnumWithImageEditorProperty (when
//                                   images are present) or just enum
//   BitfieldEditorProperty       ~ BGSBitfieldEditorProperty
//   Point2EditorProperty         ~ BGSBasePoint2EditorProperty
//   Point3EditorProperty         ~ BGSBasePoint3EditorProperty
//   MinMaxEditorProperty         ~ BGSMinMaxEditorProperty
//   TypedFormValuePairEditorProperty ~ BGTypedFormValuePairEditorProperty
//                                   (a form ID plus a scalar, used
//                                    in containers / leveled lists)
//
// This is the minimum set we need for Tier 1+2 components; we add
// the rest in Tier 3 (e.g. TemplateEditorProperty for leveled lists).

#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>

#include <memory>
#include <vector>

class Component;

class EditorProperty
{
public:
    virtual ~EditorProperty() = default;

    // The display label shown in the property grid next to the
    // editor widget. For example "Editor ID", "Model File Name",
    // "Value".
    virtual QString name() const = 0;

    // The current value of the property, in a type that's safe to
    // put in a QVariant. Subclasses pick the type — bool, int, float,
    // QString, quint32 (form ID), QVector<quint32>, etc.
    virtual QVariant value() const = 0;

    // Set the property's value. The variant must be convertible to
    // the property's natural type; mismatches are logged and ignored.
    virtual void setValue(const QVariant& v) = 0;

    // Optional: a tooltip shown on hover. Default: empty.
    virtual QString toolTip() const { return {}; }

    // Equality: two properties are equal when their values compare
    // equal. Used by the merge-with-existing logic to skip writing
    // unchanged properties.
    virtual bool isEqualTo(const EditorProperty* other) const
    {
        return other && value() == other->value();
    }
};

// =============================================================================
// Concrete property types.
// =============================================================================

class BoolEditorProperty : public EditorProperty
{
public:
    BoolEditorProperty(QString name, bool* storage)
        : m_name(std::move(name)), m_storage(storage) {}

    QString name() const override { return m_name; }
    QVariant value() const override { return m_storage && *m_storage; }
    void setValue(const QVariant& v) override
    {
        if (m_storage) *m_storage = v.toBool();
    }

private:
    QString m_name;
    bool* m_storage;
};

class IntEditorProperty : public EditorProperty
{
public:
    IntEditorProperty(QString name, qint32* storage)
        : m_name(std::move(name)), m_storage(storage) {}

    QString name() const override { return m_name; }
    QVariant value() const override { return m_storage ? QVariant(*m_storage) : QVariant(); }
    void setValue(const QVariant& v) override
    {
        if (m_storage) *m_storage = v.toInt();
    }

private:
    QString m_name;
    qint32* m_storage;
};

class UIntEditorProperty : public EditorProperty
{
public:
    UIntEditorProperty(QString name, quint32* storage)
        : m_name(std::move(name)), m_storage(storage) {}

    QString name() const override { return m_name; }
    QVariant value() const override { return m_storage ? QVariant(*m_storage) : QVariant(); }
    void setValue(const QVariant& v) override
    {
        if (m_storage) *m_storage = v.toUInt();
    }

private:
    QString m_name;
    quint32* m_storage;
};

class FloatEditorProperty : public EditorProperty
{
public:
    FloatEditorProperty(QString name, float* storage)
        : m_name(std::move(name)), m_storage(storage) {}

    QString name() const override { return m_name; }
    QVariant value() const override { return m_storage ? QVariant(*m_storage) : QVariant(); }
    void setValue(const QVariant& v) override
    {
        if (m_storage) *m_storage = v.toFloat();
    }

private:
    QString m_name;
    float* m_storage;
};

class StringEditorProperty : public EditorProperty
{
public:
    StringEditorProperty(QString name, QString* storage)
        : m_name(std::move(name)), m_storage(storage) {}

    QString name() const override { return m_name; }
    QVariant value() const override { return m_storage ? QVariant(*m_storage) : QVariant(); }
    void setValue(const QVariant& v) override
    {
        if (m_storage) *m_storage = v.toString();
    }

private:
    QString m_name;
    QString* m_storage;
};

// A form-ID picker property. We store a 32-bit form ID and let the
// property grid host a specialized picker widget when it knows how
// to render one; otherwise it falls back to a QSpinBox.
class FormEditorProperty : public EditorProperty
{
public:
    FormEditorProperty(QString name, quint32* storage)
        : m_name(std::move(name)), m_storage(storage) {}

    QString name() const override { return m_name; }
    QVariant value() const override { return m_storage ? QVariant(*m_storage) : QVariant(0u); }
    void setValue(const QVariant& v) override
    {
        if (m_storage) *m_storage = v.toUInt();
    }

private:
    QString m_name;
    quint32* m_storage;
};

// An array of form IDs (e.g. the list of keywords a form carries).
// The grid renders this as a table of form pickers with add/remove
// buttons.
class FormArrayEditorProperty : public EditorProperty
{
public:
    FormArrayEditorProperty(QString name, QVector<quint32>* storage)
        : m_name(std::move(name)), m_storage(storage) {}

    QString name() const override { return m_name; }
    QVariant value() const override
    {
        QVariantList list;
        if (m_storage)
        {
            for (quint32 id : *m_storage) list.append(id);
        }
        return list;
    }
    void setValue(const QVariant& v) override
    {
        if (!m_storage) return;
        QVariantList list = v.toList();
        m_storage->clear();
        m_storage->reserve(list.size());
        for (const QVariant& item : list) m_storage->append(item.toUInt());
    }

private:
    QString m_name;
    QVector<quint32>* m_storage;
};

// A bitmask editor property. Renders as a group of checkboxes,
// one per bit in the mask. The label list defines each checkbox's
// display name and which bit it toggles.
struct BitfieldDef {
    const char* label;
    quint32 mask;
};

class BitfieldEditorProperty : public EditorProperty
{
public:
    BitfieldEditorProperty(QString name, quint32* storage,
                           std::vector<BitfieldDef> bits)
        : m_name(std::move(name)), m_storage(storage), m_bits(std::move(bits))
    {}

    QString name() const override { return m_name; }
    QVariant value() const override { return m_storage ? QVariant(*m_storage) : QVariant(0u); }
    void setValue(const QVariant& v) override
    {
        if (m_storage) *m_storage = v.toUInt();
    }

    const std::vector<BitfieldDef>& bits() const { return m_bits; }

private:
    QString m_name;
    quint32* m_storage;
    std::vector<BitfieldDef> m_bits;
};

// An enum dropdown property. Maps integer values to display strings.
class EnumEditorProperty : public EditorProperty
{
public:
    struct Entry { QString label; quint32 value; };

    EnumEditorProperty(QString name, quint32* storage,
                       std::vector<Entry> entries)
        : m_name(std::move(name)), m_storage(storage), m_entries(std::move(entries))
    {}

    QString name() const override { return m_name; }
    QVariant value() const override { return m_storage ? QVariant(*m_storage) : QVariant(0u); }
    void setValue(const QVariant& v) override
    {
        if (m_storage) *m_storage = v.toUInt();
    }

    const std::vector<Entry>& entries() const { return m_entries; }

private:
    QString m_name;
    quint32* m_storage;
    std::vector<Entry> m_entries;
};

// A color picker property. Stores RGBA as four floats.
class ColorEditorProperty : public EditorProperty
{
public:
    ColorEditorProperty(QString name, float* r, float* g, float* b, float* a = nullptr)
        : m_name(std::move(name)), m_r(r), m_g(g), m_b(b), m_a(a)
    {}

    QString name() const override { return m_name; }
    QVariant value() const override
    {
        QVariantList v;
        v << (m_r ? *m_r : 0.0f) << (m_g ? *m_g : 0.0f)
          << (m_b ? *m_b : 0.0f) << (m_a ? *m_a : 1.0f);
        return v;
    }
    void setValue(const QVariant& v) override
    {
        QVariantList list = v.toList();
        if (list.size() >= 1 && m_r) *m_r = list[0].toFloat();
        if (list.size() >= 2 && m_g) *m_g = list[1].toFloat();
        if (list.size() >= 3 && m_b) *m_b = list[2].toFloat();
        if (list.size() >= 4 && m_a) *m_a = list[3].toFloat();
    }

private:
    QString m_name;
    float* m_r;
    float* m_g;
    float* m_b;
    float* m_a;
};

class Point2EditorProperty : public EditorProperty
{
public:
    Point2EditorProperty(QString name, float* x, float* y)
        : m_name(std::move(name)), m_x(x), m_y(y) {}

    QString name() const override { return m_name; }
    QVariant value() const override
    {
        QVariantList v;
        v << (m_x ? *m_x : 0.0f) << (m_y ? *m_y : 0.0f);
        return v;
    }
    void setValue(const QVariant& v) override
    {
        QVariantList list = v.toList();
        if (list.size() >= 1 && m_x) *m_x = list[0].toFloat();
        if (list.size() >= 2 && m_y) *m_y = list[1].toFloat();
    }

private:
    QString m_name;
    float* m_x;
    float* m_y;
};

class Point3EditorProperty : public EditorProperty
{
public:
    Point3EditorProperty(QString name, float* x, float* y, float* z)
        : m_name(std::move(name)), m_x(x), m_y(y), m_z(z) {}

    QString name() const override { return m_name; }
    QVariant value() const override
    {
        QVariantList v;
        v << (m_x ? *m_x : 0.0f) << (m_y ? *m_y : 0.0f) << (m_z ? *m_z : 0.0f);
        return v;
    }
    void setValue(const QVariant& v) override
    {
        QVariantList list = v.toList();
        if (list.size() >= 1 && m_x) *m_x = list[0].toFloat();
        if (list.size() >= 2 && m_y) *m_y = list[1].toFloat();
        if (list.size() >= 3 && m_z) *m_z = list[2].toFloat();
    }

private:
    QString m_name;
    float* m_x;
    float* m_y;
    float* m_z;
};

class MinMaxEditorProperty : public EditorProperty
{
public:
    MinMaxEditorProperty(QString name, float* min, float* max)
        : m_name(std::move(name)), m_min(min), m_max(max) {}

    QString name() const override { return m_name; }
    QVariant value() const override
    {
        QVariantList v;
        v << (m_min ? *m_min : 0.0f) << (m_max ? *m_max : 0.0f);
        return v;
    }
    void setValue(const QVariant& v) override
    {
        QVariantList list = v.toList();
        if (list.size() >= 1 && m_min) *m_min = list[0].toFloat();
        if (list.size() >= 2 && m_max) *m_max = list[1].toFloat();
    }

private:
    QString m_name;
    float* m_min;
    float* m_max;
};

#endif // EDITOR_PROPERTY_HPP
