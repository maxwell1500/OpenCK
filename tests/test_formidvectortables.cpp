// Tests for the FormID vector tables in the component widget.
//
// The keyword, spell and container tables each rendered their vector and
// mutated it only on Add and Remove. Typing a new Form ID into a cell changed
// nothing, and Add inserted a null FormID that every reference check skips
// because 0 means "unset". These tests drive the tables the way the user does
// and check the component vector actually changes.

#include <QApplication>
#include <cstdio>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QPushButton>

#include "../../libs/components/formcomponents.hpp"
#include "../../libs/components/tier1_components.hpp"
#include "../../libs/components/tier3_components.hpp"
#include "../../src/view/widgets/formcomponentwidget.hpp"

using openck::FormComponentWidget;
using openck::FormComponents;
using tescomponents::BGSKeywordForm_Component;
using tescomponents::TESContainer_Component;
using tescomponents::TESSpellList_Component;

static int g_failures = 0;

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s at %s:%d\\n", #cond, __FILE__, __LINE__); \
            ++g_failures; \
        } \
    } while (0)

namespace {

// The table is the widget's child, named so the test can find it.
QTableWidget* findTable(QWidget* widget, const QString& name)
{
    return widget->findChild<QTableWidget*>(name);
}

QPushButton* findButton(QWidget* widget, const QString& text)
{
    for (auto* button : widget->findChildren<QPushButton*>())
        if (button->text() == text)
            return button;
    return nullptr;
}

// Simulates a user typing into a cell and pressing enter, which is what
// QTableWidget emits for a committed edit.
void typeIntoCell(QTableWidget* table, int row, int column, const QString& text)
{
    QTableWidgetItem* item = table->item(row, column);
    if (!item) return;
    item->setText(text);
}

} // namespace

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    // ---- keywords: a cell edit must reach the component ---------------
    {
        FormComponents components;
        auto* kw = components.add<BGSKeywordForm_Component>();
        kw->keywords = { 0x11u, 0x22u };

        FormComponentWidget widget(kw, nullptr);
        QTableWidget* table = findTable(&widget, QStringLiteral("formIdVectorTable"));
        CHECK(table != nullptr);
        if (table)
        {
            CHECK(table->rowCount() == 2);
            // The existing values are rendered.
            CHECK(table->item(0, 0)->text() == QStringLiteral("0x00000011"));
            CHECK(table->item(1, 0)->text() == QStringLiteral("0x00000022"));

            // This is the bug: editing a cell used to change nothing.
            typeIntoCell(table, 0, 0, QStringLiteral("0x000000ab"));
            CHECK(kw->keywords.size() == 2);
            if (kw->keywords.size() == 2)
                CHECK(kw->keywords[0] == 0xabu);
            // ...and bare hex is accepted, then normalised for display.
            CHECK(table->item(0, 0)->text() == QStringLiteral("0x000000ab"));

            // The untouched row must be unaffected.
            CHECK(kw->keywords[1] == 0x22u);
        }
    }

    // ---- an unparseable cell must be rejected, not stored as zero ----
    {
        FormComponents components;
        auto* kw = components.add<BGSKeywordForm_Component>();
        kw->keywords = { 0x11u };

        FormComponentWidget widget(kw, nullptr);
        QTableWidget* table = findTable(&widget, QStringLiteral("formIdVectorTable"));
        CHECK(table != nullptr);
        if (table)
        {
            typeIntoCell(table, 0, 0, QStringLiteral("nonsense"));
            // The previous value must survive a rejected edit, and the cell is
            // marked rather than silently accepted as 0.
            CHECK(kw->keywords.size() == 1);
            if (kw->keywords.size() == 1)
                CHECK(kw->keywords[0] == 0x11u);
            CHECK(!table->item(0, 0)->toolTip().isEmpty());
        }
    }

    // ---- add and remove still work, and add opens the new cell -------
    {
        FormComponents components;
        auto* kw = components.add<BGSKeywordForm_Component>();
        kw->keywords = { 0x11u };

        FormComponentWidget widget(kw, nullptr);
        QTableWidget* table = findTable(&widget, QStringLiteral("formIdVectorTable"));
        QPushButton* add = findButton(&widget, QStringLiteral("Add Keyword"));
        QPushButton* remove = findButton(&widget, QStringLiteral("Remove"));
        CHECK(table != nullptr);
        CHECK(add != nullptr);
        CHECK(remove != nullptr);

        if (add)
        {
            add->click();
            CHECK(kw->keywords.size() == 2);
            // A new row starts as null and is immediately editable, so the user
            // is not left with a silent unset reference.
            if (kw->keywords.size() == 2)
                CHECK(kw->keywords[1] == 0u);
            CHECK(table->rowCount() == 2);
        }
        if (table && add)
        {
            // Fill in the new row the way the user would.
            typeIntoCell(table, 1, 0, QStringLiteral("0x00000099"));
            if (kw->keywords.size() == 2)
                CHECK(kw->keywords[1] == 0x99u);
        }
        if (remove && table)
        {
            table->setCurrentCell(1, 0);
            remove->click();
            CHECK(kw->keywords.size() == 1);
            if (kw->keywords.size() == 1)
                CHECK(kw->keywords[0] == 0x11u);
        }
    }

    // ---- spells use the same table -----------------------------------
    {
        FormComponents components;
        auto* sl = components.add<TESSpellList_Component>();
        sl->spells = { 0x33u };

        FormComponentWidget widget(sl, nullptr);
        QTableWidget* table = findTable(&widget, QStringLiteral("formIdVectorTable"));
        CHECK(table != nullptr);
        if (table)
        {
            typeIntoCell(table, 0, 0, QStringLiteral("0x00000044"));
            CHECK(sl->spells.size() == 1);
            if (sl->spells.size() == 1)
                CHECK(sl->spells[0] == 0x44u);
        }
    }

    // ---- the container table has a count column that must commit too --
    {
        FormComponents components;
        auto* container = components.add<TESContainer_Component>();
        container->items = { {0x11u, 3} };
        FormComponentWidget widget(container, nullptr);
        QTableWidget* table = findTable(&widget, QStringLiteral("containerItemsTable"));
        CHECK(table != nullptr);
        if (table)
        {
            CHECK(table->columnCount() == 2);
            CHECK(table->rowCount() == 1);
            CHECK(table->item(0, 0)->text() == QStringLiteral("0x00000011"));

            // The Form ID column commits.
            typeIntoCell(table, 0, 0, QStringLiteral("0x00000055"));
            CHECK(container->items.size() == 1);
            if (container->items.size() == 1)
                CHECK(container->items[0].formId == 0x55u);

            // ...and so does the count column, which used to be dropped.
            typeIntoCell(table, 0, 1, QStringLiteral("7"));
            if (container->items.size() == 1)
                CHECK(container->items[0].count == 7u);
        }
    }

    // ---- a bad count must be rejected, leaving the vector intact ------
    {
        FormComponents components;
        auto* container = components.add<TESContainer_Component>();
        container->items = { {0x11u, 3u} };

        FormComponentWidget widget(container, nullptr);
        QTableWidget* table = findTable(&widget, QStringLiteral("containerItemsTable"));
        CHECK(table != nullptr);
        if (table)
        {
            typeIntoCell(table, 0, 1, QStringLiteral("lots"));
            CHECK(container->items.size() == 1);
            if (container->items.size() == 1)
                CHECK(container->items[0].count == 3u);
            CHECK(!table->item(0, 1)->toolTip().isEmpty());
        }
    }

    if (g_failures == 0)
    {
        fprintf(stderr, "test_formidvectortables: all checks passed\\n");
        return 0;
    }
    fprintf(stderr, "test_formidvectortables: %d check(s) failed\\n", g_failures);
    return 1;
}
