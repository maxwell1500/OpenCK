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
#include <QVariant>
#include <QVector>

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

#endif // EDITOR_PROPERTY_HPP
