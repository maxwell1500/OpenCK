#ifndef DIALDATAWIDGET_HPP
#define DIALDATAWIDGET_HPP

#include "../widgets/formdatawidget.hpp"

#include <QWidget>

class QLineEdit;
class QListWidget;

namespace openck {

class FormComponents;

class DialDataWidget : public QWidget, public FormDataWidget
{
    Q_OBJECT

public:
    explicit DialDataWidget(void* recordPtr, FormComponents* components,
                            QWidget* parent = nullptr);
    ~DialDataWidget() override;

    void loadSession() override;
    bool validateSession(QString* error) override;
    void applySession() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // DIALDATAWIDGET_HPP
