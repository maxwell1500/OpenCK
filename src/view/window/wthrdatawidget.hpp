#ifndef WTHRDATAWIDGET_HPP
#define WTHRDATAWIDGET_HPP

#include "../widgets/formdatawidget.hpp"

#include <QWidget>

class QLineEdit;
class QSpinBox;

namespace openck {

class FormComponents;

class WthrDataWidget : public QWidget, public FormDataWidget
{
    Q_OBJECT

public:
    explicit WthrDataWidget(void* recordPtr, FormComponents* components,
                            QWidget* parent = nullptr);
    ~WthrDataWidget() override;

    void loadSession() override;
    bool validateSession(QString* error) override;
    void applySession() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // WTHRDATAWIDGET_HPP
