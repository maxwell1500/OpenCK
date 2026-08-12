// Unit tests for QtFormDialog and QtFormDialogManager.
//
// Covers:
//  * Creating a dialog with FormComponents renders one section per
//    component.
//  * openOrFocus deduplicates: opening the same key twice focuses the
//    existing dialog rather than creating a second one.
//  * closeAll() closes every open dialog.
//  * registerFactory() / hasFactory() register and detect a custom
//    widget factory for a record type.
//
// Uses QApplication + manual main (no QTEST_MAIN) so we can spin a
// real event loop and inspect widget state.

#include <QApplication>
#include <QDialog>
#include <QLabel>
#include <QList>

#include "../../libs/components/component.hpp"
#include "../../libs/components/editorproperty.hpp"
#include "../../libs/components/formcomponents.hpp"
#include "../../libs/components/tesfullname.hpp"
#include "../../libs/components/tier1_components.hpp"
#include "../../libs/components/tier2_components.hpp"

#include "../../src/view/window/qtformdialog.hpp"
#include "../../src/view/window/qtformdialogmanager.hpp"
#include "../../src/view/widgets/editorpropertygrid.hpp"
#include "../../src/view/widgets/formcomponentwidget.hpp"

using openck::FormComponents;
using openck::QtFormDialog;
using openck::QtFormDialogManager;
using openck::EditorPropertyGrid;
using tescomponents::TESFullName_Component;
using tescomponents::TESModel_Component;
using tescomponents::BGSKeywordForm_Component;
using tescomponents::TESEnchantableForm_Component;

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            qWarning() << "FAIL:" << #cond << "at" << __FILE__ << ":" << __LINE__; \
            ++failures; \
        } \
    } while (0)

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    int failures = 0;

    // Ensure the singleton starts in a clean state.
    QtFormDialogManager& mgr = QtFormDialogManager::instance();
    mgr.closeAll();

    // -----------------------------------------------------------------
    // T1_QtFormDialogOpensWithSections
    // Construct a FormComponents with two components, hand them to
    // QtFormDialog, and verify the dialog wires up an
    // EditorPropertyGrid with one section per component.
    // -----------------------------------------------------------------
    {
        FormComponents components;
        TESFullName_Component* name = components.add<TESFullName_Component>();
        name->fullName = QStringLiteral("Iron Sword");
        TESModel_Component* model = components.add<TESModel_Component>();
        model->modelPath = QStringLiteral("weapons/iron/sword.nif");

        QtFormDialog dialog(QStringLiteral("0x00012345"), &components);

        CHECK(dialog.formIdKey() == QStringLiteral("0x00012345"));
        CHECK(dialog.components() == &components);
        CHECK(components.size() == static_cast<std::size_t>(2));

        QList<EditorPropertyGrid*> grids = dialog.findChildren<EditorPropertyGrid*>();
        CHECK(grids.size() == 1);
        if (grids.size() == 1)
        {
            const auto& sections = grids.first()->sections();
            CHECK(sections.size() == 2);
            if (sections.size() == 2)
            {
                CHECK(sections.at(0)->component()->name() == QStringLiteral("Name"));
                CHECK(sections.at(1)->component()->name() == QStringLiteral("Model"));
                CHECK(!sections.at(0)->properties().empty());
                CHECK(!sections.at(1)->properties().empty());
            }
        }
        CHECK(dialog.windowTitle().contains(QStringLiteral("0x00012345")));
    }

    // -----------------------------------------------------------------
    // T1b_ComponentTabSplit
    // A dialog with basic + specialized + keyword components renders
    // three grids: Basic (universal components only), Components
    // (record-specific), Keywords (the keyword form only).
    // -----------------------------------------------------------------
    {
        FormComponents components;
        components.clear();
        TESFullName_Component* name = components.add<TESFullName_Component>();
        name->fullName = QStringLiteral("Enchanted Sword");
        TESModel_Component* model = components.add<TESModel_Component>();
        model->modelPath = QStringLiteral("weapons/iron/sword.nif");
        BGSKeywordForm_Component* kw = components.add<BGSKeywordForm_Component>();
        kw->keywords = { 0x12345678, 0x1234ABCD };
        TESEnchantableForm_Component* ench = components.add<TESEnchantableForm_Component>();
        ench->enchantmentFormId = 0x0000DEAD;

        QtFormDialog dialog(QStringLiteral("0x00054321"), &components);

        QList<EditorPropertyGrid*> grids = dialog.findChildren<EditorPropertyGrid*>();
        CHECK(grids.size() == 3);
        if (grids.size() == 3)
        {
            const auto& basicSections = grids.at(0)->sections();
            CHECK(basicSections.size() == 2);
            if (basicSections.size() == 2)
            {
                CHECK(basicSections.at(0)->component()->className() == QStringLiteral("TESFullName"));
                CHECK(basicSections.at(1)->component()->className() == QStringLiteral("TESModel"));
            }

            const auto& compSections = grids.at(1)->sections();
            CHECK(compSections.size() == 1);
            if (compSections.size() == 1)
                CHECK(compSections.at(0)->component()->className() == QStringLiteral("TESEnchantableForm"));

            const auto& kwSections = grids.at(2)->sections();
            CHECK(kwSections.size() == 1);
            if (kwSections.size() == 1)
                CHECK(kwSections.at(0)->component()->className() == QStringLiteral("BGSKeywordForm"));
        }
    }

    // -----------------------------------------------------------------
    // T2_OpenOrFocusDeduplicates
    // Calling openOrFocus twice with the same formIdKey must not
    // create a second dialog. The manager's openCount stays at 1 and
    // the existing dialog is shown again.
    // -----------------------------------------------------------------
    {
        mgr.closeAll();
        CHECK(mgr.openCount() == 0);

        static FormComponents compsA;
        compsA.clear();
        compsA.add<TESFullName_Component>();

        mgr.openOrFocus(QStringLiteral("0x000AAAA"), &compsA);
        CHECK(mgr.openCount() == 1);

        mgr.openOrFocus(QStringLiteral("0x000AAAA"), &compsA);
        CHECK(mgr.openCount() == 1);

        static FormComponents compsB;
        compsB.clear();
        compsB.add<TESModel_Component>();

        mgr.openOrFocus(QStringLiteral("0x000BBBB"), &compsB);
        CHECK(mgr.openCount() == 2);

        mgr.openOrFocus(QStringLiteral("0x000AAAA"), &compsA);
        CHECK(mgr.openCount() == 2);
    }

    // -----------------------------------------------------------------
    // T3_CloseAllClosesEveryDialog
    // After closeAll(), the manager reports zero open dialogs.
    // -----------------------------------------------------------------
    {
        mgr.closeAll();
        CHECK(mgr.openCount() == 0);

        static FormComponents c1;
        c1.clear();
        c1.add<TESFullName_Component>();
        static FormComponents c2;
        c2.clear();
        c2.add<TESModel_Component>();

        mgr.openOrFocus(QStringLiteral("0x00CC01"), &c1);
        mgr.openOrFocus(QStringLiteral("0x00CC02"), &c2);
        CHECK(mgr.openCount() == 2);

        mgr.closeAll();
        CHECK(mgr.openCount() == 0);
    }

    // -----------------------------------------------------------------
    // T4_RegisterAndQueryFactory
    // registerFactory() stores a factory under a record-type key and
    // hasFactory() reports true only for registered keys.
    // -----------------------------------------------------------------
    {
        mgr.closeAll();

        CHECK(!mgr.hasFactory(QStringLiteral("NPC_")));
        CHECK(!mgr.hasFactory(QStringLiteral("STAT")));

        mgr.registerFactory(QStringLiteral("NPC_"),
            [](FormComponents*, void*, QWidget* parent) -> QWidget* {
                return new QLabel(QStringLiteral("NPC custom widget"), parent);
            });

        CHECK(mgr.hasFactory(QStringLiteral("NPC_")));
        CHECK(!mgr.hasFactory(QStringLiteral("STAT")));

        mgr.registerFactory(QStringLiteral("STAT"),
            [](FormComponents*, void*, QWidget* parent) -> QWidget* {
                return new QLabel(QStringLiteral("STAT custom widget"), parent);
            });

        CHECK(mgr.hasFactory(QStringLiteral("STAT")));
    }

    // -----------------------------------------------------------------
    // T5_OpenOrFocusWithFactoryCreatesDialogAndDedups
    // openOrFocus with a registered record type should still create a
    // dialog and dedup on the formIdKey. The factory is invoked
    // during open; we verify by tracking an invocation counter.
    // -----------------------------------------------------------------
    {
        mgr.closeAll();

        static FormComponents c;
        c.clear();
        c.add<TESFullName_Component>();

        static int factoryCalls = 0;
        mgr.registerFactory(QStringLiteral("FACTTEST"),
            [](FormComponents*, void*, QWidget* parent) -> QWidget* {
                ++factoryCalls;
                return new QLabel(QStringLiteral("Factory widget"), parent);
            });

        CHECK(factoryCalls == 0);
        mgr.openOrFocus(QStringLiteral("0x000FF01"), QStringLiteral("FACTTEST"), &c);
        CHECK(mgr.openCount() == 1);
        CHECK(factoryCalls == 1);

        // Second open with the same key must NOT call the factory again.
        mgr.openOrFocus(QStringLiteral("0x000FF01"), QStringLiteral("FACTTEST"), &c);
        CHECK(mgr.openCount() == 1);
        CHECK(factoryCalls == 1);

        mgr.closeAll();
        CHECK(mgr.openCount() == 0);
    }

    if (failures == 0)
    {
        qDebug() << "test_qtformdialog: all checks passed";
        return 0;
    }
    qWarning() << "test_qtformdialog:" << failures << "check(s) failed";
    return 1;
}