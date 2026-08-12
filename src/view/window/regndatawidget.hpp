#ifndef REGNDATAWIDGET_HPP
#define REGNDATAWIDGET_HPP

#include <QWidget>

class QLineEdit;
class QSpinBox;

namespace openck {

class FormComponents;

class RegnDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RegnDataWidget(void* recordPtr, FormComponents* components,
                            QWidget* parent = nullptr);
    ~RegnDataWidget() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // REGNDATAWIDGET_HPP
