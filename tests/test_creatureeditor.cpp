// Tests for the CREA (Creature) editor widget and its QtFormDialogManager
// factory registration.
//
// Covers:
//  * CreatureDataWidget renders Editor ID, Full Name and the key CreaData
//    numeric fields from a CreatureRecord.
//  * The widget builds a layout even with a null record pointer.
//  * registerFactory("CREA") + hasFactory("CREA") wire up the widget, and
//    openOrFocus opens a dialog with the factory (deduplicated by form ID).

#include <QApplication>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>

#include "../../libs/files/esm/creaturerecord.hpp"
#include "../../libs/components/formcomponents.hpp"
#include "../../src/view/window/creatureeditor.hpp"
#include "../../src/view/window/qtformdialogmanager.hpp"

using openck::CreatureDataWidget;
using openck::FormComponents;
using openck::QtFormDialogManager;

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
    // T1_FieldsRenderFromRecord
    // A CreatureDataWidget built from a valid record shows the editor
    // ID, full name, type, vitals, damage and all eight attributes.
    // -----------------------------------------------------------------
    {
        CreatureRecord rec;
        rec.editorId = QStringLiteral("TestCreature");
        rec.fullName = QStringLiteral("Test Creature");
        rec.creaData.type = 3;
        rec.creaData.health = 100;
        rec.creaData.magicka = 60;
        rec.creaData.fatigue = 120;
        rec.creaData.damage = 25;
        rec.creaData.strength = 40;
        rec.creaData.intelligence = 30;
        rec.creaData.willpower = 35;
        rec.creaData.agility = 45;
        rec.creaData.speed = 50;
        rec.creaData.endurance = 55;
        rec.creaData.personality = 20;
        rec.creaData.luck = 15;

        CreatureDataWidget w(&rec, &rec.components);
        CHECK(w.layout() != nullptr);

        auto groups = w.findChildren<QGroupBox*>();
        CHECK(groups.size() == 2);

        auto* idEdit = w.findChild<QLineEdit*>(QStringLiteral("editorId"));
        auto* nameEdit = w.findChild<QLineEdit*>(QStringLiteral("fullName"));
        CHECK(idEdit && idEdit->text() == QStringLiteral("TestCreature"));
        CHECK(nameEdit && nameEdit->text() == QStringLiteral("Test Creature"));

        auto spins = w.findChildren<QSpinBox*>();
        CHECK(spins.size() == 13);
        if (spins.size() == 13)
        {
            CHECK(spins.at(0)->value() == 3);
            CHECK(spins.at(1)->value() == 100);
            CHECK(spins.at(2)->value() == 60);
            CHECK(spins.at(3)->value() == 120);
            CHECK(spins.at(4)->value() == 25);
            CHECK(spins.at(5)->value() == 40);
            CHECK(spins.at(6)->value() == 30);
            CHECK(spins.at(7)->value() == 35);
            CHECK(spins.at(8)->value() == 45);
            CHECK(spins.at(9)->value() == 50);
            CHECK(spins.at(10)->value() == 55);
            CHECK(spins.at(11)->value() == 20);
            CHECK(spins.at(12)->value() == 15);
        }
    }

    // -----------------------------------------------------------------
    // T2_NullRecordBuildsLayout
    // With a null record pointer the widget still builds its layout
    // without touching the record.
    // -----------------------------------------------------------------
    {
        CreatureDataWidget w(nullptr, nullptr);
        CHECK(w.layout() != nullptr);
        CHECK(!w.findChildren<QGroupBox*>().isEmpty());
    }

    // -----------------------------------------------------------------
    // T3_FactoryRegistrationAndDialogOpen
    // Registering the CREA factory makes hasFactory("CREA") true, and
    // openOrFocus creates a dialog that is deduplicated by form ID.
    // -----------------------------------------------------------------
    {
        QtFormDialogManager& mgr = QtFormDialogManager::instance();
        mgr.closeAll();

        CHECK(!mgr.hasFactory(QStringLiteral("CREA")));

        mgr.registerFactory(QStringLiteral("CREA"),
            [](FormComponents*, void* recPtr, QWidget* parent) -> QWidget* {
                return new CreatureDataWidget(recPtr, nullptr, parent);
            });

        CHECK(mgr.hasFactory(QStringLiteral("CREA")));

        static CreatureRecord rec;
        rec.editorId = QStringLiteral("CliffRacer");
        rec.formId = 0x1234;
        rec.fullName = QStringLiteral("Cliff Racer");
        rec.creaData.health = 75;

        mgr.openOrFocus(QStringLiteral("0x00001234"), QStringLiteral("CREA"),
                        &rec.components, &rec);
        CHECK(mgr.openCount() == 1);

        mgr.openOrFocus(QStringLiteral("0x00001234"), QStringLiteral("CREA"),
                        &rec.components, &rec);
        CHECK(mgr.openCount() == 1);

        mgr.openOrFocus(QStringLiteral("0x00005678"), QStringLiteral("CREA"),
                        &rec.components, &rec);
        CHECK(mgr.openCount() == 2);

        mgr.closeAll();
        CHECK(mgr.openCount() == 0);
    }

    if (failures == 0)
    {
        qDebug() << "test_creatureeditor: all checks passed";
        return 0;
    }
    qWarning() << "test_creatureeditor:" << failures << "check(s) failed";
    return 1;
}
