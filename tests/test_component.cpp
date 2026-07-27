// Unit tests for the FormComponents container and the Component
// abstract base (libs/components). Exercises the container's
// add/find/clear/size/all helpers plus the operator== deep comparison.

#include <QtTest>
#include <QString>

#include "../../libs/components/component.hpp"
#include "../../libs/components/editorproperty.hpp"
#include "../../libs/components/formcomponents.hpp"
#include "../../libs/components/tesfullname.hpp"
#include "../../libs/components/tier1_components.hpp"
#include "../../libs/components/tier2_components.hpp"

using openck::FormComponents;
using tescomponents::TESFullName_Component;
using tescomponents::TESModel_Component;
using tescomponents::TESHealth_Component;
using tescomponents::BGSKeywordForm_Component;

class TestComponent : public QObject
{
    Q_OBJECT

private slots:
    void AddCreatesAndReturnsComponent();
    void FindByNameReturnsCorrectComponent();
    void FindByNameReturnsNullForMissing();
    void ClearRemovesAllComponents();
    void SizeReturnsCorrectCount();
    void AllReturnsVector();
    void OperatorEqualComparesComponentData();
    void OperatorEqualDifferentSizeIsFalse();
    void CopyConstructProducesDeepClone();
};

void TestComponent::AddCreatesAndReturnsComponent()
{
    FormComponents components;
    QVERIFY(components.empty());

    TESFullName_Component* name = components.add<TESFullName_Component>();
    QVERIFY(name != nullptr);
    QCOMPARE(components.size(), static_cast<std::size_t>(1));
    QVERIFY(!components.empty());

    TESModel_Component* model = components.add<TESModel_Component>();
    QVERIFY(model != nullptr);
    QCOMPARE(components.size(), static_cast<std::size_t>(2));
}

void TestComponent::FindByNameReturnsCorrectComponent()
{
    FormComponents components;
    components.add<TESFullName_Component>();
    TESModel_Component* model = components.add<TESModel_Component>();
    model->modelPath = QStringLiteral("weapons/sword.nif");

    Component* found = components.findByName(QStringLiteral("TESModel"));
    QVERIFY(found != nullptr);
    QCOMPARE(found->className(), QStringLiteral("TESModel"));
    auto* typed = static_cast<TESModel_Component*>(found);
    QCOMPARE(typed->modelPath, QStringLiteral("weapons/sword.nif"));

    Component* fullName = components.findByName(QStringLiteral("TESFullName"));
    QVERIFY(fullName != nullptr);
    QCOMPARE(fullName->className(), QStringLiteral("TESFullName"));
}

void TestComponent::FindByNameReturnsNullForMissing()
{
    FormComponents components;
    components.add<TESFullName_Component>();

    QVERIFY(components.findByName(QStringLiteral("TESModel")) == nullptr);
    QVERIFY(components.findByName(QStringLiteral("Nonexistent")) == nullptr);
}

void TestComponent::ClearRemovesAllComponents()
{
    FormComponents components;
    components.add<TESFullName_Component>();
    components.add<TESModel_Component>();
    components.add<TESHealth_Component>();
    QCOMPARE(components.size(), static_cast<std::size_t>(3));

    components.clear();
    QCOMPARE(components.size(), static_cast<std::size_t>(0));
    QVERIFY(components.empty());
    QVERIFY(components.findByName(QStringLiteral("TESFullName")) == nullptr);
}

void TestComponent::SizeReturnsCorrectCount()
{
    FormComponents components;
    QCOMPARE(components.size(), static_cast<std::size_t>(0));

    for (int i = 0; i < 5; ++i)
        components.add<TESHealth_Component>();
    QCOMPARE(components.size(), static_cast<std::size_t>(5));

    components.clear();
    QCOMPARE(components.size(), static_cast<std::size_t>(0));
}

void TestComponent::AllReturnsVector()
{
    FormComponents components;
    components.add<TESFullName_Component>();
    components.add<TESModel_Component>();

    const auto& vec = components.all();
    QCOMPARE(vec.size(), static_cast<std::size_t>(2));
    QVERIFY(vec.at(0) != nullptr);
    QVERIFY(vec.at(1) != nullptr);
    QCOMPARE(vec.at(0)->className(), QStringLiteral("TESFullName"));
    QCOMPARE(vec.at(1)->className(), QStringLiteral("TESModel"));
}

void TestComponent::OperatorEqualComparesComponentData()
{
    FormComponents a;
    auto* aName = a.add<TESFullName_Component>();
    aName->fullName = QStringLiteral("Iron Sword");
    auto* aModel = a.add<TESModel_Component>();
    aModel->modelPath = QStringLiteral("iron/sword.nif");

    FormComponents b;
    auto* bName = b.add<TESFullName_Component>();
    bName->fullName = QStringLiteral("Iron Sword");
    auto* bModel = b.add<TESModel_Component>();
    bModel->modelPath = QStringLiteral("iron/sword.nif");

    QVERIFY(a == b);
    QVERIFY(!(a != b));

    bName->fullName = QStringLiteral("Steel Sword");
    QVERIFY(!(a == b));
    QVERIFY(a != b);
}

void TestComponent::OperatorEqualDifferentSizeIsFalse()
{
    FormComponents a;
    a.add<TESFullName_Component>();

    FormComponents b;
    b.add<TESFullName_Component>();
    b.add<TESModel_Component>();

    QVERIFY(!(a == b));
    QVERIFY(a != b);
}

void TestComponent::CopyConstructProducesDeepClone()
{
    FormComponents original;
    auto* name = original.add<TESFullName_Component>();
    name->fullName = QStringLiteral("Original");

    FormComponents copy(original);
    QCOMPARE(copy.size(), original.size());

    Component* copyName = copy.findByName(QStringLiteral("TESFullName"));
    QVERIFY(copyName != nullptr);
    auto* typed = static_cast<TESFullName_Component*>(copyName);
    QCOMPARE(typed->fullName, QStringLiteral("Original"));

    name->fullName = QStringLiteral("Mutated");
    QVERIFY(typed->fullName == QStringLiteral("Original"));
}

QTEST_MAIN(TestComponent)
#include "test_component.moc"