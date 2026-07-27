// Unit tests for the Component lifecycle helpers defined on the
// abstract Component base: createEditorProperties, clone, copyFrom,
// isEqualTo, and mergeWith. These exercise the polymorphic contract
// every Tier 1/2/3 component must satisfy.

#include <QtTest>
#include <QString>
#include <QVector>

#include "../../libs/components/component.hpp"
#include "../../libs/components/editorproperty.hpp"
#include "../../libs/components/formcomponents.hpp"
#include "../../libs/components/tesfullname.hpp"
#include "../../libs/components/tier1_components.hpp"
#include "../../libs/components/tier2_components.hpp"
#include "../../libs/components/tier3_components.hpp"

using tescomponents::TESFullName_Component;
using tescomponents::TESModel_Component;
using tescomponents::TESHealth_Component;
using tescomponents::TESValue_Component;
using tescomponents::TESWeight_Component;
using tescomponents::BGSKeywordForm_Component;
using tescomponents::TESContainer_Component;
using tescomponents::TESBipedModel_Component;
using tescomponents::TypedFormValuePair;

class TestEditorLifecycle : public QObject
{
    Q_OBJECT

private slots:
    void CreateEditorPropertiesReturnsCorrectCount();
    void CloneProducesDeepCopy();
    void CopyFromCopiesData();
    void CopyFromNullOrMismatchedIsNoOp();
    void IsEqualToComparesCorrectly();
    void MergeWithMergesData();
    void KeywordFormMergeAppendsUnique();
    void ContainerMergeAppendsUnique();
};

void TestEditorLifecycle::CreateEditorPropertiesReturnsCorrectCount()
{
    TESFullName_Component name;
    auto nameProps = name.createEditorProperties();
    QCOMPARE(nameProps.size(), static_cast<std::size_t>(1));

    TESModel_Component model;
    auto modelProps = model.createEditorProperties();
    QCOMPARE(modelProps.size(), static_cast<std::size_t>(2));

    TESHealth_Component health;
    auto healthProps = health.createEditorProperties();
    QCOMPARE(healthProps.size(), static_cast<std::size_t>(1));

    TESBipedModel_Component biped;
    auto bipedProps = biped.createEditorProperties();
    QCOMPARE(bipedProps.size(), static_cast<std::size_t>(3));

    // TESContainer has no editable properties yet.
    TESContainer_Component container;
    auto containerProps = container.createEditorProperties();
    QCOMPARE(containerProps.size(), static_cast<std::size_t>(0));
}

void TestEditorLifecycle::CloneProducesDeepCopy()
{
    TESModel_Component original;
    original.modelPath = QStringLiteral("original.nif");
    original.lodModelPath = QStringLiteral("original_lod.nif");

    auto copy = original.clone();
    QVERIFY(copy != nullptr);
    QCOMPARE(copy->className(), original.className());

    auto* typed = static_cast<TESModel_Component*>(copy.get());
    QCOMPARE(typed->modelPath, QStringLiteral("original.nif"));
    QCOMPARE(typed->lodModelPath, QStringLiteral("original_lod.nif"));

    original.modelPath = QStringLiteral("changed.nif");
    QVERIFY(typed->modelPath == QStringLiteral("original.nif"));
}

void TestEditorLifecycle::CopyFromCopiesData()
{
    TESHealth_Component target;
    target.health = 10.0f;

    TESHealth_Component source;
    source.health = 250.5f;

    target.copyFrom(&source);
    QCOMPARE(target.health, 250.5f);
}

void TestEditorLifecycle::CopyFromNullOrMismatchedIsNoOp()
{
    TESModel_Component target;
    target.modelPath = QStringLiteral("keep.nif");

    target.copyFrom(nullptr);
    QCOMPARE(target.modelPath, QStringLiteral("keep.nif"));

    TESFullName_Component wrongType;
    wrongType.fullName = QStringLiteral("ignored");
    target.copyFrom(&wrongType);
    QCOMPARE(target.modelPath, QStringLiteral("keep.nif"));
}

void TestEditorLifecycle::IsEqualToComparesCorrectly()
{
    TESValue_Component a;
    a.value = 100;
    TESValue_Component b;
    b.value = 100;
    QVERIFY(a.isEqualTo(&b));

    b.value = 200;
    QVERIFY(!a.isEqualTo(&b));

    QVERIFY(!a.isEqualTo(nullptr));

    TESWeight_Component otherClass;
    QVERIFY(!a.isEqualTo(&otherClass));
}

void TestEditorLifecycle::MergeWithMergesData()
{
    TESHealth_Component dst;
    dst.health = 10.0f;
    TESHealth_Component src;
    src.health = 99.0f;

    dst.mergeWith(&src);
    QCOMPARE(dst.health, 99.0f);
}

void TestEditorLifecycle::KeywordFormMergeAppendsUnique()
{
    BGSKeywordForm_Component a;
    a.keywords.append(0x100u);
    a.keywords.append(0x200u);

    BGSKeywordForm_Component b;
    b.keywords.append(0x200u); // duplicate
    b.keywords.append(0x300u); // new

    a.mergeWith(&b);

    QCOMPARE(a.keywords.size(), 3);
    QVERIFY(a.keywords.contains(0x100u));
    QVERIFY(a.keywords.contains(0x200u));
    QVERIFY(a.keywords.contains(0x300u));
}

void TestEditorLifecycle::ContainerMergeAppendsUnique()
{
    TESContainer_Component a;
    TypedFormValuePair e1{0xAAAA, 1};
    TypedFormValuePair e2{0xBBBB, 2};
    a.items.append(e1);
    a.items.append(e2);

    TESContainer_Component b;
    TypedFormValuePair e2dup{0xBBBB, 2}; // duplicate of existing
    TypedFormValuePair e3{0xCCCC, 3};    // new
    b.items.append(e2dup);
    b.items.append(e3);

    a.mergeWith(&b);

    QCOMPARE(a.items.size(), 3);
    QVERIFY(a.items.contains(e1));
    QVERIFY(a.items.contains(e2));
    QVERIFY(a.items.contains(e3));
}

QTEST_MAIN(TestEditorLifecycle)
#include "test_editor_lifecycle.moc"