#include "editorpropertygrid.hpp"
#include "formcomponentwidget.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace openck {

EditorPropertyGrid::EditorPropertyGrid(QWidget* parent)
    : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(8);
    m_layout->addStretch(1);
}

EditorPropertyGrid::~EditorPropertyGrid() = default;

void EditorPropertyGrid::setComponents(const std::vector<Component*>& components)
{
    // Clear out any existing sections. We delete the widgets and
    // remove the rows but preserve the trailing stretch.
    for (FormComponentWidget* w : m_sections)
    {
        m_layout->removeWidget(w);
        w->deleteLater();
    }
    m_sections.clear();

    // Insert the new sections, in order, before the trailing
    // stretch (which is the last child of m_layout).
    int insertAt = m_layout->count() - 1;
    if (insertAt < 0) insertAt = 0;

    for (Component* c : components)
    {
        if (!c) continue;
        auto* section = new FormComponentWidget(c, this);
        m_layout->insertWidget(insertAt++, section);
        m_sections.push_back(section);
    }
}

void EditorPropertyGrid::refresh()
{
    for (FormComponentWidget* w : m_sections)
    {
        w->refresh();
    }
}

bool EditorPropertyGrid::apply()
{
    bool anyChanged = false;
    for (FormComponentWidget* w : m_sections)
    {
        anyChanged |= w->apply();
    }
    return anyChanged;
}

} // namespace openck
