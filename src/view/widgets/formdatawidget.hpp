#ifndef FORMDATAWIDGET_HPP
#define FORMDATAWIDGET_HPP

#include <QString>

namespace openck {

/// Optional contract for a custom data widget hosted in a QtFormDialog.
///
/// A widget that only reads its record needs nothing from this. A widget that
/// lets the user change things should derive from it so the dialog can drive
/// the edit the same way it drives the component grid: loadSession() when the
/// session opens, validateSession() before anything is committed, and
/// applySession() to push the widget's edits into the session's working copy.
///
/// The widget is constructed with the session's working record, never the live
/// one, so its edits are discarded for free on Cancel. applySession() is for
/// the part a working copy cannot cover on its own — pulling the widget's
/// controls back into the record, or resolving values that only make sense at
/// commit time.
class FormDataWidget
{
public:
    virtual ~FormDataWidget() = default;

    /// Populates the widget from the session's working record. Called once,
    /// after the widget is installed.
    virtual void loadSession() {}

    /// Returns false to refuse the commit, with a reason for the user.
    virtual bool validateSession(QString* error) { Q_UNUSED(error); return true; }

    /// Writes the widget's edits into the session's working record.
    virtual void applySession() {}
};

} // namespace openck

#endif // FORMDATAWIDGET_HPP
