// Qt-based test for the QtFormDialog — the top-level record editor
// dialog. We construct a FormComponents container with a few
// components, hand it to QtFormDialog, and verify the dialog wires up
// the underlying EditorPropertyGrid with one section per component.

#include <QApplication>
#include <QDialog>
#include <QList>
#include <QLabel>

#include "../../libs/components/component.hpp"
#include "../../libs/components/editorproperty.hpp"
#include "../../libs/components/formcomponents.hpp"
#include "../../libs/components/tesfullname.hpp"
#include "../../libs/components/tier1_components.hpp"

#include "../../src/view/window/qtformdialog.hpp"
#include "../../src/view/widgets/editorpropertygrid.hpp"
#include "../../src/view/widgets/formcomponentwidget.hpp"

using openck::FormComponents;
using openck::QtFormDialog;
using openck::EditorPropertyGrid;
using openck::FormComponentWidget;
using tescomponents::TESFullName_Component;
using tescomponents::TESModel_Component;

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

    // -----------------------------------------------------------------
    // T6_QtFormDialogOpens
    // Construct a FormComponents with two components, hand them to
    // QtFormDialog, and verify the dialog has the expected property
    // grid sections.
    // -----------------------------------------------------------------
    {
        FormComponents components;
        TESFullName_Component* name = components.add<TESFullName_Component>();
        name->fullName = QStringLiteral("Iron Sword");
        TESModel_Component* model = components.add<TESModel_Component>();
        model->modelPath = QStringLiteral("weapons/iron/sword.nif");

        QtFormDialog dialog(QStringLiteral("0x00012345"), &components);
        dialog.show();

        // Sanity: the dialog remembers the form key and components.
        CHECK(dialog.formIdKey() == QStringLiteral("0x00012345"));
        CHECK(dialog.components() == &components);
        CHECK(components.size() == 2);

        // The dialog must have created a property grid with one
        // section per component. We find the grid by type and walk
        // its sections.
        QList<EditorPropertyGrid*> grids = dialog.findChildren<EditorPropertyGrid*>();
        CHECK(grids.size() == 1);
        if (grids.size() == 1)
        {
            const auto& sections = grids.first()->sections();
            CHECK(sections.size() == 2);
            if (sections.size() == 2)
            {
                // First section is the FullName component (Name).
                CHECK(sections.at(0)->component()
                    == static_cast<Component*>(&components.all().at(0)));
                CHECK(sections.at(0)->component()->name()
                    == QStringLiteral("Name"));

                // Second section is the Model component.
                CHECK(sections.at(1)->component()
                    == static_cast<Component*>(&components.all().at(1)));
                CHECK(sections.at(1)->component()->name()
                    == QStringLiteral("Model"));

                // Each section must have produced a non-empty list
                // of editor properties.
                CHECK(!sections.at(0)->properties().empty());
                CHECK(!sections.at(1)->properties().empty());
            }
        }

        // The dialog's window title should embed the form key.
        CHECK(dialog.windowTitle().contains(QStringLiteral("0x00012345")));
    }

    // -----------------------------------------------------------------
    // T6_QtFormDialogClosesOnCancel
    // Drive the dialog via show() + reject() and verify the result is
    // QDialog::Rejected. We can't call exec() in a unit test (it
    // blocks the event loop); show() + reject() is the standard way
    // to verify the rejection path without blocking.
    // -----------------------------------------------------------------
    {
        FormComponents components;
        components.add<TESFullName_Component>();
        components.add<TESModel_Component>();

        QtFormDialog dialog(QStringLiteral("0x00099999"), &components);
        dialog.show();

        // Initially the dialog is open; the result is 0 (QDialog::Rejected
        // by default before exec is called).
        CHECK(dialog.result() == 0);

        // Calling reject() must mark the dialog as QDialog::Rejected.
        dialog.reject();
        CHECK(dialog.result() == static_cast<int>(QDialog::Rejected));
    }

    if (failures == 0)
    {
        qDebug() << "test_qtformdialog: all checks passed";
        return 0;
    }
    qWarning() << "test_qtformdialog:" << failures << "check(s) failed";
    return 1;
}
