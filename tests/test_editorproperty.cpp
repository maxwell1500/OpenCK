// Unit tests for every concrete EditorProperty type in
// libs/components/editorproperty.hpp. Each property type is exercised
// through its value() / setValue() round-trip against the underlying
// storage variable. No Qt widget code is needed — these are pure
// data-binding tests.

#include <QtTest>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVector>

#include "../../libs/components/editorproperty.hpp"

class TestEditorProperty : public QObject
{
    Q_OBJECT

private slots:
    void BoolEditorPropertyGetSet();
    void IntEditorPropertyGetSet();
    void FloatEditorPropertyGetSet();
    void StringEditorPropertyGetSet();
    void FormEditorPropertyGetSet();
    void FormArrayEditorPropertyGetSet();
    void BitfieldEditorPropertyGetSet();
    void EnumEditorPropertyGetSet();
    void ColorEditorPropertyGetSet();
    void Point2EditorPropertyGetSet();
    void Point3EditorPropertyGetSet();
    void MinMaxEditorPropertyGetSet();
};

void TestEditorProperty::BoolEditorPropertyGetSet()
{
    bool storage = false;
    BoolEditorProperty prop(QStringLiteral("Flag"), &storage);

    QCOMPARE(prop.name(), QStringLiteral("Flag"));
    QCOMPARE(prop.value().toBool(), false);
    prop.setValue(true);
    QCOMPARE(storage, true);
    QCOMPARE(prop.value().toBool(), true);
    prop.setValue(false);
    QCOMPARE(prop.value().toBool(), false);
    QCOMPARE(storage, false);
}

void TestEditorProperty::IntEditorPropertyGetSet()
{
    qint32 storage = 0;
    IntEditorProperty prop(QStringLiteral("Count"), &storage);

    QCOMPARE(prop.value().toInt(), 0);
    prop.setValue(42);
    QCOMPARE(storage, 42);
    QCOMPARE(prop.value().toInt(), 42);
    prop.setValue(-7);
    QCOMPARE(prop.value().toInt(), -7);
    QCOMPARE(storage, -7);
}

void TestEditorProperty::FloatEditorPropertyGetSet()
{
    float storage = 0.0f;
    FloatEditorProperty prop(QStringLiteral("Weight"), &storage);

    QCOMPARE(prop.value().toFloat(), 0.0f);
    prop.setValue(3.14f);
    QCOMPARE(storage, 3.14f);
    QCOMPARE(prop.value().toFloat(), 3.14f);
    prop.setValue(-2.5f);
    QCOMPARE(prop.value().toFloat(), -2.5f);
}

void TestEditorProperty::StringEditorPropertyGetSet()
{
    QString storage;
    StringEditorProperty prop(QStringLiteral("Editor ID"), &storage);

    QCOMPARE(prop.value().toString(), QString());
    prop.setValue(QStringLiteral("MyItem"));
    QCOMPARE(storage, QStringLiteral("MyItem"));
    QCOMPARE(prop.value().toString(), QStringLiteral("MyItem"));
    prop.setValue(QString());
    QCOMPARE(prop.value().toString(), QString());
}

void TestEditorProperty::FormEditorPropertyGetSet()
{
    quint32 storage = 0;
    FormEditorProperty prop(QStringLiteral("Pickup Sound"), &storage);

    QCOMPARE(prop.value().toUInt(), 0u);
    prop.setValue(0xDEADBEEFu);
    QCOMPARE(storage, 0xDEADBEEFu);
    QCOMPARE(prop.value().toUInt(), 0xDEADBEEFu);
}

void TestEditorProperty::FormArrayEditorPropertyGetSet()
{
    QVector<quint32> storage;
    FormArrayEditorProperty prop(QStringLiteral("Keywords"), &storage);

    QCOMPARE(prop.value().toList().size(), 0);

    QVariantList list;
    list.append(0x100u);
    list.append(0x200u);
    list.append(0x300u);
    prop.setValue(list);
    QCOMPARE(storage.size(), 3);
    QCOMPARE(storage.at(0), 0x100u);
    QCOMPARE(storage.at(1), 0x200u);
    QCOMPARE(storage.at(2), 0x300u);

    QVariantList back = prop.value().toList();
    QCOMPARE(back.size(), 3);
    QCOMPARE(back.at(0).toUInt(), 0x100u);

    QVariantList trimmed;
    trimmed.append(0x100u);
    prop.setValue(trimmed);
    QCOMPARE(storage.size(), 1);
    QCOMPARE(storage.at(0), 0x100u);
}

void TestEditorProperty::BitfieldEditorPropertyGetSet()
{
    quint32 storage = 0;
    std::vector<BitfieldDef> bits = {
        {"Bit0", 0x1u},
        {"Bit1", 0x2u},
        {"Bit2", 0x4u},
    };
    BitfieldEditorProperty prop(QStringLiteral("Flags"), &storage, bits);

    QCOMPARE(prop.value().toUInt(), 0u);
    QCOMPARE(prop.bits().size(), static_cast<std::size_t>(3));
    QCOMPARE(prop.bits().at(0).label, "Bit0");
    QCOMPARE(prop.bits().at(2).mask, 0x4u);

    prop.setValue(0x5u);
    QCOMPARE(storage, 0x5u);
    QCOMPARE(prop.value().toUInt(), 0x5u);
}

void TestEditorProperty::EnumEditorPropertyGetSet()
{
    quint32 storage = 0;
    std::vector<EnumEditorProperty::Entry> entries = {
        {QStringLiteral("Zero"), 0u},
        {QStringLiteral("One"),  1u},
        {QStringLiteral("Two"),  2u},
    };
    EnumEditorProperty prop(QStringLiteral("Aggression"), &storage, entries);

    QCOMPARE(prop.value().toUInt(), 0u);
    QCOMPARE(prop.entries().size(), static_cast<std::size_t>(3));
    QCOMPARE(prop.entries().at(1).label, QStringLiteral("One"));

    prop.setValue(2u);
    QCOMPARE(storage, 2u);
    QCOMPARE(prop.value().toUInt(), 2u);
}

void TestEditorProperty::ColorEditorPropertyGetSet()
{
    float r = 0, g = 0, b = 0, a = 1;
    ColorEditorProperty prop(QStringLiteral("Tint"), &r, &g, &b, &a);

    QVariantList initial = prop.value().toList();
    QCOMPARE(initial.size(), 4);
    QCOMPARE(initial.at(0).toFloat(), 0.0f);
    QCOMPARE(initial.at(3).toFloat(), 1.0f);

    QVariantList newColor;
    newColor << 0.1f << 0.2f << 0.3f << 0.4f;
    prop.setValue(newColor);
    QCOMPARE(r, 0.1f);
    QCOMPARE(g, 0.2f);
    QCOMPARE(b, 0.3f);
    QCOMPARE(a, 0.4f);

    QVariantList back = prop.value().toList();
    QCOMPARE(back.at(0).toFloat(), 0.1f);
    QCOMPARE(back.at(3).toFloat(), 0.4f);
}

void TestEditorProperty::Point2EditorPropertyGetSet()
{
    float x = 0, y = 0;
    Point2EditorProperty prop(QStringLiteral("UV Offset"), &x, &y);

    QVariantList initial = prop.value().toList();
    QCOMPARE(initial.size(), 2);

    QVariantList v;
    v << 1.5f << -2.5f;
    prop.setValue(v);
    QCOMPARE(x, 1.5f);
    QCOMPARE(y, -2.5f);

    QVariantList back = prop.value().toList();
    QCOMPARE(back.at(0).toFloat(), 1.5f);
    QCOMPARE(back.at(1).toFloat(), -2.5f);
}

void TestEditorProperty::Point3EditorPropertyGetSet()
{
    float x = 0, y = 0, z = 0;
    Point3EditorProperty prop(QStringLiteral("Position"), &x, &y, &z);

    QVariantList initial = prop.value().toList();
    QCOMPARE(initial.size(), 3);

    QVariantList v;
    v << 10.0f << 20.0f << 30.0f;
    prop.setValue(v);
    QCOMPARE(x, 10.0f);
    QCOMPARE(y, 20.0f);
    QCOMPARE(z, 30.0f);

    QVariantList back = prop.value().toList();
    QCOMPARE(back.at(0).toFloat(), 10.0f);
    QCOMPARE(back.at(2).toFloat(), 30.0f);
}

void TestEditorProperty::MinMaxEditorPropertyGetSet()
{
    float min = 0, max = 0;
    MinMaxEditorProperty prop(QStringLiteral("Range"), &min, &max);

    QVariantList initial = prop.value().toList();
    QCOMPARE(initial.size(), 2);

    QVariantList v;
    v << -5.0f << 25.0f;
    prop.setValue(v);
    QCOMPARE(min, -5.0f);
    QCOMPARE(max, 25.0f);

    QVariantList back = prop.value().toList();
    QCOMPARE(back.at(0).toFloat(), -5.0f);
    QCOMPARE(back.at(1).toFloat(), 25.0f);
}

QTEST_MAIN(TestEditorProperty)
#include "test_editorproperty.moc"