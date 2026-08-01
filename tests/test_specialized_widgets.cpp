#include <QtTest>
#include <QApplication>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>

#include "../../src/view/window/packdatawidget.hpp"
#include "../../src/view/window/worldspacedatawidget.hpp"
#include "../../src/view/window/locationdatawidget.hpp"
#include "../../libs/files/esm/Packagerecord.hpp"
#include "../../libs/files/esm/worldspacerecord.hpp"
#include "../../libs/files/esm/locationrecord.hpp"
#include "../../libs/components/formcomponents.hpp"

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

    {
        PackageRecord rec;
        rec.editorId = QStringLiteral("TestPackage");
        rec.packageType = 2;
        rec.targetType = 3;
        rec.flags = 5;
        rec.targetIds = { 0x1234, 0x5678 };

        PackDataWidget w(&rec, &rec.components);
        CHECK(w.layout() != nullptr);
        auto* idEdit = w.findChild<QLineEdit*>(QStringLiteral("editorId"));
        CHECK(idEdit && idEdit->text() == QStringLiteral("TestPackage"));
        auto spins = w.findChildren<QSpinBox*>();
        CHECK(spins.size() == 3);
        if (spins.size() == 3)
        {
            CHECK(spins.at(0)->value() == 2);
            CHECK(spins.at(1)->value() == 3);
            CHECK(spins.at(2)->value() == 5);
        }
        auto* list = w.findChild<QListWidget*>();
        CHECK(list && list->count() == 2);
    }

    {
        WorldspaceRecord rec;
        rec.editorId = QStringLiteral("Tamriel");
        rec.name = QStringLiteral("Tamriel");
        rec.waterType = 1;
        rec.climateId = 2;
        rec.lightingId = 3;
        rec.music = 4;
        rec.terrain = 5;

        WorldspaceDataWidget w(&rec, &rec.components);
        CHECK(w.layout() != nullptr);
        auto* idEdit = w.findChild<QLineEdit*>(QStringLiteral("editorId"));
        CHECK(idEdit && idEdit->text() == QStringLiteral("Tamriel"));
        auto* nameEdit = w.findChild<QLineEdit*>(QStringLiteral("name"));
        CHECK(nameEdit && nameEdit->text() == QStringLiteral("Tamriel"));
        auto spins = w.findChildren<QSpinBox*>();
        CHECK(spins.size() == 5);
        if (spins.size() == 5)
        {
            CHECK(spins.at(0)->value() == 1);
            CHECK(spins.at(1)->value() == 2);
            CHECK(spins.at(2)->value() == 3);
            CHECK(spins.at(3)->value() == 4);
            CHECK(spins.at(4)->value() == 5);
        }
    }

    {
        LocationRecord rec;
        rec.editorId = QStringLiteral("Riverwood");
        rec.locationName = QStringLiteral("Riverwood");
        rec.parentId = 7;
        rec.x = 10;
        rec.y = 20;
        rec.z = 30;

        LocationDataWidget w(&rec, &rec.components);
        CHECK(w.layout() != nullptr);
        auto* idEdit = w.findChild<QLineEdit*>(QStringLiteral("editorId"));
        CHECK(idEdit && idEdit->text() == QStringLiteral("Riverwood"));
        auto* nameEdit = w.findChild<QLineEdit*>(QStringLiteral("name"));
        CHECK(nameEdit && nameEdit->text() == QStringLiteral("Riverwood"));
        auto spins = w.findChildren<QSpinBox*>();
        CHECK(spins.size() == 4);
        if (spins.size() == 4)
        {
            CHECK(spins.at(0)->value() == 7);
            CHECK(spins.at(1)->value() == 10);
            CHECK(spins.at(2)->value() == 20);
            CHECK(spins.at(3)->value() == 30);
        }
    }

    if (failures == 0)
    {
        qDebug() << "test_specialized_widgets: all checks passed";
        return 0;
    }
    qWarning() << "test_specialized_widgets:" << failures << "check(s) failed";
    return 1;
}
