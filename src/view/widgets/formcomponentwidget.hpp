#ifndef FORM_COMPONENT_WIDGET_HPP
#define FORM_COMPONENT_WIDGET_HPP

// FormComponentWidget — a QWidget that hosts the property editor rows
// for one Component. The component creates its EditorProperty list,
// the widget lays them out in a QFormLayout (label + editor), and
// exposes a `component()` accessor for the editor to commit changes
// back via the component's API.

#include "../libs/components/component.hpp"
#include "../libs/components/editorproperty.hpp"

#include <QWidget>

#include <memory>
#include <vector>

class QFormLayout;
class QLabel;

namespace openck {

class FormComponentWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FormComponentWidget(Component* component, QWidget* parent = nullptr);
    ~FormComponentWidget() override;

    Component* component() const { return m_component; }
    const Component* component() const { return m_component; }

    // Returns the editor properties the widget was constructed with,
    // so the surrounding grid can wire up batch-edit operations
    // (e.g. select all matching records and edit the same property
    // across all of them).
    const std::vector<std::unique_ptr<EditorProperty>>& properties() const
    {
        return m_properties;
    }

    // Re-binds the editor widgets to the current property values.
    // Used after the component is mutated externally (e.g. via an
    // undo/redo) to refresh the visible state.
    void refresh();

    // Pulls the current editor values back into the underlying
    // component storage. Returns true if any value changed.
    bool apply();

private:
    Component* m_component;
    std::vector<std::unique_ptr<EditorProperty>> m_properties;
    QFormLayout* m_layout = nullptr;
};

} // namespace openck

#endif // FORM_COMPONENT_WIDGET_HPP
