#ifndef CLASSDATAWIDGET_HPP
#define CLASSDATAWIDGET_HPP

#include <QWidget>

class QLineEdit;
class QSpinBox;

namespace openck {

class FormComponents;

class ClassDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ClassDataWidget(void* recordPtr, FormComponents* components,
                             QWidget* parent = nullptr);
    ~ClassDataWidget() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // CLASSDATAWIDGET_HPP
