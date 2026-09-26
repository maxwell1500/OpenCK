#ifndef CLASSDATAWIDGET_HPP
#define CLASSDATAWIDGET_HPP

#include "../widgets/formdatawidget.hpp"

#include <QWidget>

class QLineEdit;
class QSpinBox;

namespace openck {

class FormComponents;

class ClassDataWidget : public QWidget, public FormDataWidget
{
    Q_OBJECT

public:
    explicit ClassDataWidget(void* recordPtr, FormComponents* components,
                             QWidget* parent = nullptr);
    ~ClassDataWidget() override;

    void loadSession() override;
    bool validateSession(QString* error) override;
    void applySession() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // CLASSDATAWIDGET_HPP
