#ifndef QTFORMDIALOG_HPP
#define QTFORMDIALOG_HPP

// QtFormDialog — the generic record editor. Mirrors the real CK's
// QtTESFormDialog / QtCreationKitFormDialog (see
// docs/CK_Real_Integration_Plan.md). The dialog takes a pointer to
// any record type that exposes a `components` member of type
// openck::FormComponents, walks each component, and renders its
// properties in a property grid.
//
// The component grid is split into tabs matching the real CK form
// dialog: "Basic" (universal components: name/model/icon/value/weight/
// health/description), "Components" (record-specific components) and
// "Keywords" (shown only when a keyword form is present). "Data" hosts
// the per-record custom widget when one is registered.
//
// One dialog instance per record. The QtFormDialogManager
// (qtformdialogmanager.hpp) keeps a registry so opening the same
// record twice focuses the existing dialog rather than creating a
// duplicate — same behavior as the real CK.

#include "../widgets/editorpropertygrid.hpp"
#include "../widgets/formdatawidget.hpp"
#include "../../libs/components/component.hpp"
#include "../../libs/components/formcomponents.hpp"
#include "recordeditsession.hpp"

#include <QDialog>
#include <QString>

#include <memory>
#include <vector>
#include <functional>

class QFormLayout;
class QVBoxLayout;
class Data;

class QPushButton;
class QTabWidget;
class QWidget;

namespace openck {

/// Generic record editor dialog that renders a record's component properties in a grid.
class QtFormDialog : public QDialog
{
    Q_OBJECT

public:
    QtFormDialog(const QString& formIdKey, FormComponents* components,
                  QWidget* parent = nullptr,
                  std::function<void(const FormComponents&)> commit = {},
                  Data* data = nullptr,
                  std::unique_ptr<RecordEditSession> session = nullptr);
    ~QtFormDialog() override;

    QString formIdKey() const { return m_formIdKey; }
    FormComponents* components() const { return m_sourceComponents; }
    FormComponents* workingComponents() const { return m_components; }

    /// The session this dialog edits through, if it was given one.
    RecordEditSession* session() const { return m_session.get(); }

    /// Sets an optional custom widget shown below the component property grid.
    void setCustomWidget(QWidget* widget);

    /// Overridden to discard the working copy, so closing the window discards
    /// the edit exactly as Cancel does.
    void reject() override;

private slots:
    void onApply();
    void onOk();

private:
    bool commitChanges();

    QString m_formIdKey;
    FormComponents* m_sourceComponents = nullptr;
    FormComponents m_ownedWorkingComponents;
    FormComponents* m_components = nullptr;
    Data* m_data = nullptr;
    std::function<void(const FormComponents&)> m_commit;
    std::unique_ptr<RecordEditSession> m_session;
    QVBoxLayout* m_layout = nullptr;
    QTabWidget* m_tabs = nullptr;
    EditorPropertyGrid* m_basicGrid = nullptr;
    EditorPropertyGrid* m_componentsGrid = nullptr;
    EditorPropertyGrid* m_keywordsGrid = nullptr;
    QWidget* m_dataTab = nullptr;
    QVBoxLayout* m_dataTabLayout = nullptr;
    QWidget* m_customWidget = nullptr;
    FormDataWidget* m_customSession = nullptr;
};

} // namespace openck

#endif // QTFORMDIALOG_HPP
