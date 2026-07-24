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
#include "component.hpp"
#include "formcomponents.hpp"

#include <QDialog>
#include <QString>

#include <vector>

class QFormLayout;
class QVBoxLayout;
class QPushButton;

namespace openck {

class QtFormDialog : public QDialog
{
    Q_OBJECT

public:
    QtFormDialog(const QString& formIdKey, FormComponents* components,
                 QWidget* parent = nullptr);
    ~QtFormDialog() override;

    // The form ID key used to deduplicate dialogs. This is the
    // string form of the record's form ID, e.g. "0x00012345".
    QString formIdKey() const { return m_formIdKey; }

    // The form components the dialog is editing. The dialog does
    // not own these — the caller does. They typically live on the
    // record that was opened.
    FormComponents* components() const { return m_components; }

private slots:
    void onApply();
    void onOk();

private:
    QString m_formIdKey;
    FormComponents* m_components;
    EditorPropertyGrid* m_grid = nullptr;
};

} // namespace openck

#endif // QTFORMDIALOG_HPP
