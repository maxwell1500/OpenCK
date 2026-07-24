// Qt-based UI tests for the property grid widgets: FormComponentWidget
// and EditorPropertyGrid. These tests construct real Qt widgets, so a
// QApplication must be alive. We drive edits through the editor widget
// signal path and verify the underlying EditorProperty storage changes
// as expected.

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QCheckBox>

#include "../../libs/components/component.hpp"
#include "../../libs/components/editorproperty.hpp"
#include "../../libs/components/formcomponents.hpp"
#include "../../libs/components/tesfullname.hpp"
#include "../../libs/components/tier1_components.hpp"

#include "../../src/view/widgets/formcomponentwidget.hpp"
#include "../../src/view/widgets/editorpropertygrid.hpp"

using tescomponents::TESModel_Component;
using tescomponents::TESFullName_Component;
using tescomponents::TESHealth_Component;
using openck::FormComponentWidget;
using openck::EditorPropertyGrid;

// Helper: find the editor widget corresponding to a property name in a
// FormComponentWidget's QFormLayout. The form layout puts the property
// name in the label column ("Foo:") and the editor in the field column.
static QWidget* findEditorForProperty(FormComponentWidget* w, const QString& propName)
{
    if (!w) return nullptr;
    QFormLayout* fl = w->findChild<QFormLayout*>();
    if (!fl) return nullptr;
    for (int i = 0; i < fl->rowCount(); ++i)
    {
        QLayoutItem* labelItem = fl->itemAt(i, QFormLayout::LabelRole);
        if (!labelItem) continue;
        QLabel* label = qobject_cast<QLabel*>(labelItem->widget());
        if (label && label->text() == propName + QStringLiteral(":"))
        {
            QLayoutItem* fieldItem = fl->itemAt(i, QFormLayout::FieldRole);
            return fieldItem ? fieldItem->widget() : nullptr;
        }
    }
    return nullptr;
}

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
    // T4_FormComponentWidgetRenders
    // The widget must produce a bold header label for the component
    // name plus one editor widget per EditorProperty it owns.
    // -----------------------------------------------------------------
    {
        TESModel_Component model;
        FormComponentWidget widget(&model);
        widget.show();

        CHECK(widget.properties().size() == 2);
        CHECK(widget.component() == static_cast<Component*>(&model));

        QList<QLabel*> labels = widget.findChildren<QLabel*>();
        bool foundHeader = false;
        for (QLabel* l : labels)
        {
            if (l->text() == model.name())
            {
                foundHeader = true;
                QFont f = l->font();
                CHECK(f.bold());
                break;
            }
        }
        CHECK(foundHeader);

        QWidget* modelEditor = findEditorForProperty(&widget,
            QStringLiteral("Model File Name"));
        QWidget* lodEditor = findEditorForProperty(&widget,
            QStringLiteral("LOD File Name"));
        CHECK(modelEditor != nullptr);
        CHECK(lodEditor != nullptr);
    }

    // -----------------------------------------------------------------
    // T4_FormComponentWidgetReflectsEdits
    // Editing the editor widget must push the new value into the
    // underlying property storage. The model component backs its
    // modelPath with a StringEditorProperty; we drive a QLineEdit
    // edit and verify the property value follows.
    // -----------------------------------------------------------------
    {
        TESModel_Component model;
        FormComponentWidget widget(&model);
        widget.show();

        QLineEdit* modelEditor = qobject_cast<QLineEdit*>(
            findEditorForProperty(&widget, QStringLiteral("Model File Name")));
        CHECK(modelEditor != nullptr);
        if (modelEditor)
        {
            modelEditor->setText(QStringLiteral("weapons/iron/sword.nif"));
            emit modelEditor->editingFinished();
            CHECK(model.modelPath == QStringLiteral("weapons/iron/sword.nif"));

            const auto& props = widget.properties();
            CHECK(props.size() >= 1);
            if (!props.empty())
            {
                CHECK(props.front()->value().toString()
                    == QStringLiteral("weapons/iron/sword.nif"));
            }
        }
    }

    // -----------------------------------------------------------------
    // T5_EditorPropertyGridSetsMultipleComponents
    // Build a grid with two components and confirm both sections are
    // rendered.
    // -----------------------------------------------------------------
    {
        TESFullName_Component name;
        name.fullName = QStringLiteral("Sword");
        TESModel_Component model;
        model.modelPath = QStringLiteral("weapons/iron/sword.nif");

        EditorPropertyGrid grid;
        std::vector<Component*> comps;
        comps.push_back(&name);
        comps.push_back(&model);
        grid.setComponents(comps);
        grid.show();

        CHECK(grid.sections().size() == 2);
        CHECK(grid.sections().at(0)->component() == static_cast<Component*>(&name));
        CHECK(grid.sections().at(1)->component() == static_cast<Component*>(&model));

        CHECK(grid.sections().at(0)->properties().size() == 1);
        CHECK(grid.sections().at(1)->properties().size() == 2);

        TESHealth_Component health;
        health.health = 50.0f;
        std::vector<Component*> replacement;
        replacement.push_back(&health);
        grid.setComponents(replacement);
        CHECK(grid.sections().size() == 1);
        CHECK(grid.sections().at(0)->component() == static_cast<Component*>(&health));
    }

    if (failures == 0)
    {
        qDebug() << "test_editorproperty: all checks passed";
        return 0;
    }
    qWarning() << "test_editorproperty:" << failures << "check(s) failed";
    return 1;
}
