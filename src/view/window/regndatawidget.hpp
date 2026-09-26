#ifndef REGNDATAWIDGET_HPP
#define REGNDATAWIDGET_HPP

#include "../widgets/formdatawidget.hpp"

#include <QWidget>

class QLineEdit;
class QSpinBox;

namespace openck {

class FormComponents;

class RegnDataWidget : public QWidget, public FormDataWidget
{
    Q_OBJECT

public:
    explicit RegnDataWidget(void* recordPtr, FormComponents* components,
                            QWidget* parent = nullptr);
    ~RegnDataWidget() override;

    void loadSession() override;
    bool validateSession(QString* error) override;
    void applySession() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // REGNDATAWIDGET_HPP
