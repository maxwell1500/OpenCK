#ifndef WTHRDATAWIDGET_HPP
#define WTHRDATAWIDGET_HPP

#include <QWidget>

class QLineEdit;
class QSpinBox;

namespace openck {

class FormComponents;

class WthrDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WthrDataWidget(void* recordPtr, FormComponents* components,
                            QWidget* parent = nullptr);
    ~WthrDataWidget() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // WTHRDATAWIDGET_HPP
