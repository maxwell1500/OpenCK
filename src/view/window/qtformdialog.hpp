#ifndef QTFORMDIALOG_HPP
#define QTFORMDIALOG_HPP

// QtFormDialog — the generic record editor. Mirrors the real CK's
// QtTESFormDialog / QtCreationKitFormDialog (see
// docs/CK_Real_Integration_Plan.md). The dialog takes a pointer to
// any record type that exposes a `components` member of type
// openck::FormComponents, walks each component, and renders its
// properties in a property grid.
//
// One dialog instance per record. The QtFormDialogManager
// (qtformdialogmanager.hpp) keeps a registry so opening the same
// record twice focuses the existing dialog rather than creating a
// duplicate — same behavior as the real CK.

#include "../widgets/editorpropertygrid.hpp"
#include "../libs/components/component.hpp"
#include "../libs/components/formcomponents.hpp"

#include <QDialog>
#include <QString>

#include <vector>

class QFormLayout;
class QVBoxLayout;
class QPushButton;
class QTabWidget;
class QWidget;

namespace openck {

class QtFormDialog : public QDialog
{
    Q_OBJECT

public:
    QtFormDialog(const QString& formIdKey, FormComponents* components,
                 QWidget* parent = nullptr);
    ~QtFormDialog() override;

    QString formIdKey() const { return m_formIdKey; }
    FormComponents* components() const { return m_components; }

    // Set an optional custom widget that appears below the component
    // property grid. Used by complex record types (NPC, RACE, CELL, etc.)
    // to add record-specific editing sections beyond the generic grid.
    void setCustomWidget(QWidget* widget);

private slots:
    void onApply();
    void onOk();

private:
    QString m_formIdKey;
    FormComponents* m_components;
    QVBoxLayout* m_layout = nullptr;
    QTabWidget* m_tabs = nullptr;
    EditorPropertyGrid* m_grid = nullptr;
    QWidget* m_dataTab = nullptr;
    QVBoxLayout* m_dataTabLayout = nullptr;
    QWidget* m_customWidget = nullptr;
};

} // namespace openck

#endif // QTFORMDIALOG_HPP
