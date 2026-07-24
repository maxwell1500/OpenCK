#ifndef EDITOR_PROPERTY_GRID_HPP
#define EDITOR_PROPERTY_GRID_HPP

// EditorPropertyGrid — a top-level QWidget that hosts one
// FormComponentWidget per component, in a vertical layout. Each
// component's section is bracketed by a bold header label so the
// user can see the component boundaries at a glance.
//
// This is OpenCK's equivalent of the real CK's BGSEditorPropertyGrid
// (which the binary exposes under E:\BuildAgent\work\fee57674ddcb42c9\Genesis\
// Construction Set\Editor Properties\Inspector\BGSEditorPropertyGrid.cpp).
// We use a simple vertical QVBoxLayout rather than a QTreeView
// because the Tier 1+2 components we support are flat — there's no
// nested hierarchy to display. Tier 3 components (notably
// BGSPropertySheet_Component, BGSAnimationGraph_Component, etc.)
// will justify upgrading to a real QTreeWidget when we add them.

#include "../libs/components/component.hpp"

#include <QWidget>

#include <memory>
#include <vector>

class QFormLayout;
class QLabel;
class QVBoxLayout;

namespace openck {

class FormComponentWidget;

class EditorPropertyGrid : public QWidget
{
    Q_OBJECT

public:
    explicit EditorPropertyGrid(QWidget* parent = nullptr);
    ~EditorPropertyGrid() override;

    // Replaces the entire grid content with one section per
    // component. The grid takes a non-owning reference to each
    // component; ownership stays with the caller. The grid creates
    // the per-component widgets itself and parents them to this
    // widget.
    void setComponents(const std::vector<Component*>& components);

    // Returns the per-component FormComponentWidgets, in the same
    // order as setComponents(). The grid owns the widgets; callers
    // must not delete them.
    const std::vector<FormComponentWidget*>& sections() const
    {
        return m_sections;
    }

    // Refreshes the value of every editor widget by re-reading
    // each property. Used after the underlying component data is
    // mutated externally (e.g. via undo/redo).
    void refresh();

    // Pulls the editor values back into each component's storage.
    // For the current implementation the editor widgets push
    // values into storage as the user edits; this method exists
    // for symmetry and as a hook for future per-row commit
    // behavior.
    bool apply();

private:
    QVBoxLayout* m_layout = nullptr;
    std::vector<FormComponentWidget*> m_sections;
};

} // namespace openck

#endif // EDITOR_PROPERTY_GRID_HPP
