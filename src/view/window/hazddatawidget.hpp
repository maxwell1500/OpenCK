#ifndef HAZDDATAWIDGET_HPP
#define HAZDDATAWIDGET_HPP

#include <QWidget>

class QLineEdit;
class QSpinBox;

namespace openck {

class FormComponents;

class HazdDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HazdDataWidget(void* recordPtr, FormComponents* components,
                            QWidget* parent = nullptr);
    ~HazdDataWidget() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // HAZDDATAWIDGET_HPP
