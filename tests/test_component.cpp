// Unit tests for the OpenCK Component-Property architecture.
//
// Exercises the Component abstract base plus the Tier 1 (editor property
// leaf types) and Tier 2 (record component) implementations: storage
// roundtrip, subrecord dispatch, and the polymorphic helpers
// (clone / copyFrom / isEqualTo / mergeWith).

#include <QTest>
#include <QString>
#include <QVector>
#include <QVariant>
#include <QVariantList>

#include "../../libs/components/component.hpp"
#include "../../libs/components/editorproperty.hpp"
#include "../../libs/components/formcomponents.hpp"
#include "../../libs/components/tesfullname.hpp"
#include "../../libs/components/tier1_components.hpp"
#include "../../libs/components/tier2_components.hpp"

#include "../../libs/files/esm/common.hpp"

class TestComponent : public QObject
{
    Q_OBJECT

private slots:
    // Tier 1: EditorProperty leaf roundtrips.
    void T1_BoolEditorPropertyGetSet();
    void T1_IntEditorPropertyGetSet();
    void T1_FloatEditorPropertyGetSet();
    void T1_StringEditorPropertyGetSet();
    void T1_FormEditorPropertyGetSet();
    void T1_FormArrayEditorPropertyGetSet();

    // Tier 2: Component subrecord dispatch.
    void T2_TESFullNameCanHandle();
    void T2_TESFullNameHandleSubrecord();
    void T2_TESModelCanHandle();
    void T2_TESModelHandleSubrecord();
    void T2_TESHealthCanHandle();
    void T2_BGSKeywordFormCanHandle();
    void T2_BGSKeywordFormMerge();

    // Tier 3: polymorphic helpers.
    void T3_ComponentClone();
    void T3_ComponentCopyFrom();
    void T3_ComponentIsEqualTo();
};

// ---------------------------------------------------------------------------
// Tier 1
// ---------------------------------------------------------------------------

void TestComponent::T1_BoolEditorPropertyGetSet()
{
    bool storage = false;
    BoolEditorProperty prop(QStringLiteral("Flag"), &storage);

    QCOMPARE(prop.value().toBool(), false);
    prop.setValue(true);
    QCOMPARE(storage, true);
    QCOMPARE(prop.value().toBool(), true);
    prop.setValue(false);
    QCOMPARE(prop.value().toBool(), false);
    QCOMPARE(prop.name(), QStringLiteral("Flag"));
}

void TestComponent::T1_IntEditorPropertyGetSet()
{
    qint32 storage = 0;
    IntEditorProperty prop(QStringLiteral("Count"), &storage);

    QCOMPARE(prop.value().toInt(), 0);
    prop.setValue(42);
    QCOMPARE(storage, 42);
    QCOMPARE(prop.value().toInt(), 42);
    prop.setValue(-7);
    QCOMPARE(prop.value().toInt(), -7);
}

void TestComponent::T1_FloatEditorPropertyGetSet()
{
    float storage = 0.0f;
    FloatEditorProperty prop(QStringLiteral("Weight"), &storage);

    QCOMPARE(prop.value().toFloat(), 0.0f);
    prop.setValue(3.14f);
    QCOMPARE(storage, 3.14f);
    QCOMPARE(prop.value().toFloat(), 3.14f);
}

void TestComponent::T1_StringEditorPropertyGetSet()
{
    QString storage;
    StringEditorProperty prop(QStringLiteral("Editor ID"), &storage);

    QCOMPARE(prop.value().toString(), QString());
    prop.setValue(QStringLiteral("MyItem"));
    QCOMPARE(storage, QStringLiteral("MyItem"));
    QCOMPARE(prop.value().toString(), QStringLiteral("MyItem"));
}

void TestComponent::T1_FormEditorPropertyGetSet()
{
    quint32 storage = 0;
    FormEditorProperty prop(QStringLiteral("Pickup Sound"), &storage);

    QCOMPARE(prop.value().toUInt(), 0u);
    prop.setValue(0xDEADBEEFu);
    QCOMPARE(storage, 0xDEADBEEFu);
    QCOMPARE(prop.value().toUInt(), 0xDEADBEEFu);
}

void TestComponent::T1_FormArrayEditorPropertyGetSet()
{
    QVector<quint32> storage;
    FormArrayEditorProperty prop(QStringLiteral("Keywords"), &storage);

    QCOMPARE(prop.value().toList().size(), 0);

    // Add via setValue.
    QVariantList newList;
    newList.append(0x100u);
    newList.append(0x200u);
    prop.setValue(newList);
    QCOMPARE(storage.size(), 2);
    QCOMPARE(storage.at(0), 0x100u);
    QCOMPARE(storage.at(1), 0x200u);

    // "Remove" by replacing with a smaller list.
    QVariantList trimmed;
    trimmed.append(0x100u);
    prop.setValue(trimmed);
    QCOMPARE(storage.size(), 1);
    QCOMPARE(storage.at(0), 0x100u);
}

// ---------------------------------------------------------------------------
// Tier 2
// ---------------------------------------------------------------------------

void TestComponent::T2_TESFullNameCanHandle()
{
    tescomponents::TESFullName_Component comp;
    QVERIFY(comp.canHandle((NAME)'FULL'));
    QVERIFY(!comp.canHandle((NAME)'EDID'));
    QVERIFY(!comp.canHandle((NAME)'MODL'));
    QCOMPARE(comp.className(), QStringLiteral("TESFullName"));
    QCOMPARE(comp.name(), QStringLiteral("Name"));
}

void TestComponent::T2_TESFullNameHandleSubrecord()
{
    // We can't easily spin up a real ESMReader here (it requires a file
    // path), so we drive the component's slot directly with a stub
    // ESMReader constructed from a QBuffer. The key thing we want to
    // verify is that the dispatch case the production code hits (FULL
    // -> readZString) populates fullName.
    //
    // ESMReader only has a file-path constructor, so we round-trip
    // through a real file: write a tiny record, read the FULL
    // subrecord back via handleSubrecord.
    QTemporaryFile tmp;
    tmp.setAutoRemove(true);
    QVERIFY(tmp.open());
    tmp.close();

    QFile f(tmp.fileName());
    QVERIFY(f.open(QIODevice::WriteOnly));
    QDataStream out(&f);
    out.setByteOrder(QDataStream::LittleEndian);

    // Build a 5-byte FULL payload: "Hi\0\0\0" (3 chars + 2 padding)
    out.writeRawData("FULL", 4);
    out << quint16(5);
    out.writeRawData("Hi\0\0\0", 5);
    f.close();

    ESMReader reader(tmp.fileName());
    reader.open();
    NAME sub = reader.readNSubHeader();
    QCOMPARE(sub, (NAME)'FULL');

    tescomponents::TESFullName_Component comp;
    comp.handleSubrecord((NAME)'FULL', reader);
    QVERIFY(comp.fullName.startsWith(QStringLiteral("Hi")));
    QCOMPARE(reader.subLeft(), (qint64)0);
}

void TestComponent::T2_TESModelCanHandle()
{
    tescomponents::TESModel_Component comp;
    QVERIFY(comp.canHandle((NAME)'MODL'));
    QVERIFY(comp.canHandle((NAME)'ODIT'));
    QVERIFY(comp.canHandle((NAME)'MNAM'));
    QVERIFY(!comp.canHandle((NAME)'EDID'));
    QVERIFY(!comp.canHandle((NAME)'FULL'));
    QCOMPARE(comp.className(), QStringLiteral("TESModel"));
    QCOMPARE(comp.name(), QStringLiteral("Model"));
}

void TestComponent::T2_TESModelHandleSubrecord()
{
    QTemporaryFile tmp;
    tmp.setAutoRemove(true);
    QVERIFY(tmp.open());
    tmp.close();

    QFile f(tmp.fileName());
    QVERIFY(f.open(QIODevice::WriteOnly));
    QDataStream out(&f);
    out.setByteOrder(QDataStream::LittleEndian);

    // MODL subrecord with a null-terminated zstring payload.
    out.writeRawData("MODL", 4);
    out << quint16(9);
    out.writeRawData("model.nif\0", 9);
    f.close();

    ESMReader reader(tmp.fileName());
    reader.open();
    NAME sub = reader.readNSubHeader();
    QCOMPARE(sub, (NAME)'MODL');

    tescomponents::TESModel_Component comp;
    comp.handleSubrecord((NAME)'MODL', reader);
    QVERIFY(comp.modelPath.startsWith(QStringLiteral("model.nif")));
    QCOMPARE(comp.lodModelPath, QString());
}

void TestComponent::T2_TESHealthCanHandle()
{
    tescomponents::TESHealth_Component comp;
    QVERIFY(comp.canHandle((NAME)'HLTH'));
    // DATA is shared with several other components; the component
    // admits it, but dispatch order is what makes that safe.
    QVERIFY(comp.canHandle((NAME)'DATA'));
    QVERIFY(!comp.canHandle((NAME)'EDID'));
}

void TestComponent::T2_BGSKeywordFormCanHandle()
{
    tescomponents::BGSKeywordForm_Component comp;
    QVERIFY(comp.canHandle((NAME)'CNAM'));
    QVERIFY(comp.canHandle((NAME)'KWDA'));
    QVERIFY(!comp.canHandle((NAME)'EDID'));
    QCOMPARE(comp.className(), QStringLiteral("BGSKeywordForm"));
}

void TestComponent::T2_BGSKeywordFormMerge()
{
    tescomponents::BGSKeywordForm_Component a;
    a.keywords.append(0x100u);
    a.keywords.append(0x200u);

    tescomponents::BGSKeywordForm_Component b;
    b.keywords.append(0x200u);  // duplicate of an existing entry
    b.keywords.append(0x300u);  // new

    a.mergeWith(&b);

    // The merge should append 0x300u but skip 0x200u (already present).
    QCOMPARE(a.keywords.size(), 3);
    QVERIFY(a.keywords.contains(0x100u));
    QVERIFY(a.keywords.contains(0x200u));
    QVERIFY(a.keywords.contains(0x300u));
}

// ---------------------------------------------------------------------------
// Tier 3
// ---------------------------------------------------------------------------

void TestComponent::T3_ComponentClone()
{
    tescomponents::TESFullName_Component original;
    original.fullName = QStringLiteral("Original");

    auto copy = original.clone();
    QVERIFY(copy != nullptr);
    QCOMPARE(copy->className(), original.className());
    auto* typed = static_cast<tescomponents::TESFullName_Component*>(copy.get());
    QCOMPARE(typed->fullName, QStringLiteral("Original"));

    // Mutating the original must not affect the clone.
    original.fullName = QStringLiteral("Changed");
    QCOMPARE(typed->fullName, QStringLiteral("Original"));
}

void TestComponent::T3_ComponentCopyFrom()
{
    tescomponents::TESModel_Component target;
    target.modelPath = QStringLiteral("old.nif");
    target.lodModelPath = QStringLiteral("old_lod.nif");

    tescomponents::TESModel_Component source;
    source.modelPath = QStringLiteral("new.nif");
    source.lodModelPath = QStringLiteral("new_lod.nif");

    target.copyFrom(&source);

    QCOMPARE(target.modelPath, QStringLiteral("new.nif"));
    QCOMPARE(target.lodModelPath, QStringLiteral("new_lod.nif"));

    // copyFrom with a mismatched className is a no-op.
    tescomponents::TESFullName_Component unrelated;
    unrelated.fullName = QStringLiteral("FullName");
    target.copyFrom(&unrelated);
    // Unchanged:
    QCOMPARE(target.modelPath, QStringLiteral("new.nif"));

    // copyFrom with a null pointer is a no-op.
    target.copyFrom(nullptr);
    QCOMPARE(target.modelPath, QStringLiteral("new.nif"));
}

void TestComponent::T3_ComponentIsEqualTo()
{
    tescomponents::TESModel_Component a;
    a.modelPath = QStringLiteral("model.nif");
    a.lodModelPath = QStringLiteral("lod.nif");

    tescomponents::TESModel_Component b;
    b.modelPath = QStringLiteral("model.nif");
    b.lodModelPath = QStringLiteral("lod.nif");

    QVERIFY(a.isEqualTo(&b));

    b.modelPath = QStringLiteral("other.nif");
    QVERIFY(!a.isEqualTo(&b));

    // Cross-class comparison returns false.
    tescomponents::TESFullName_Component otherClass;
    otherClass.fullName = QStringLiteral("x");
    QVERIFY(!a.isEqualTo(&otherClass));

    // nullptr is not equal.
    QVERIFY(!a.isEqualTo(nullptr));
}

QTEST_MAIN(TestComponent)
#include "test_component.moc"
