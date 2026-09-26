#ifndef HAZDDATAWIDGET_HPP
#define HAZDDATAWIDGET_HPP

#include "../widgets/formdatawidget.hpp"

#include <QWidget>

class QDoubleSpinBox;
class QLineEdit;
class QSpinBox;

namespace openck {

class FormComponents;

class HazdDataWidget : public QWidget, public FormDataWidget
{
    Q_OBJECT

public:
    explicit HazdDataWidget(void* recordPtr, FormComponents* components,
                            QWidget* parent = nullptr);
    ~HazdDataWidget() override;

    void loadSession() override;
    bool validateSession(QString* error) override;
    void applySession() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // HAZDDATAWIDGET_HPP
